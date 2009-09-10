#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <new>
#include <errno.h>
#include <math.h>

#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pds/service/GenericPool.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/config/CfgClientNfs.hh"
#include "EpicsArchMonitor.hh"
#include "EpicsArchManager.hh"

using namespace Pds;
using std::string;

class EpicsArchAllocAction : public Action 
{
public:
    EpicsArchAllocAction(EpicsArchManager& manager, CfgClientNfs& cfg) : _manager(manager), _cfg(cfg) {}
    
    virtual Transition* fire(Transition* tr)     
    {
        const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
        _cfg.initialize(alloc.allocation());  
        _manager.onActionMap();
        return tr;
    }
private:
    EpicsArchManager& _manager;
    CfgClientNfs& _cfg;
};

class EpicsArchConfigAction : public Action 
{
public:
    EpicsArchConfigAction(EpicsArchManager& manager, const Src& src, CfgClientNfs& cfg) :
        //_cfgtc(_epicsArchConfigType,src),
        _manager(manager), _src(src), _cfg(cfg) 
    {}
    
    // this is the first "phase" of a transition where
    // all CPUs configure in parallel.
    virtual Transition* fire(Transition* tr) 
    {
        //_cfg.fetch(*tr,_epicsArchConfigType, &_config);
        return tr;
    }

    // this is the second "phase" of a transition where everybody
    // records the results of configure (which will typically be
    // archived in the xtc file).
    virtual InDatagram* fire(InDatagram* in) 
    {
        printf( "\n\n===== Writing Configs =====\n" );

        // insert assumes we have enough space in the input datagram
        //dg->insert(_cfgtc, &_config);
        //if (_nerror) dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        InDatagram* out = new ( _manager.getPool() ) Pds::CDatagram( in->datagram() );
        int iFail = _manager.writeMonitoredContent( out->datagram() );                
        
        /*
         * Possible failure modes for _manager.writeMonitoredContent()
         *
         * 1. Some PV's ctrl value has not been updated
         * 2. Some PV's time value has not been updated, but its ctrl value has been written out previously
         * 3. Memory pool size is not enough for storing PV data
         * 4. Other reasons. cf. EpicsMonitorPv::writeXtc()
         */
        if ( iFail != 0 )
        {
            printf( "EpicsArchConfigAction::fire(): writeMonitoredContent() failed, error code = %d\n", iFail );
            delete out;
            return in;
        }                
                  
        // !! for debug test only
        printf( "\nOutput payload size = %d\n", out->datagram().xtc.sizeofPayload());
        
        return out;
    }

private:
    //EpicsArchConfigType _config;
    //Xtc _cfgtc;
    EpicsArchManager& _manager;
    Src _src;
    CfgClientNfs& _cfg;
};

class EpicsArchL1AcceptAction : public Action 
{
public:
    EpicsArchL1AcceptAction(EpicsArchManager& manager, float fMinTriggerInterval) :
        _manager(manager) , _fMinTriggerInterval(fMinTriggerInterval)
    {
        tsPrev.tv_sec = tsPrev.tv_nsec = 0;   
    }
    
    ~EpicsArchL1AcceptAction()
    {}
    
    virtual InDatagram* fire(InDatagram* in)     
    {
        // Check delta time
        timespec tsCurrent;
        int iStatus = clock_gettime (CLOCK_REALTIME, &tsCurrent);
        if (iStatus)
        {
            printf( "EpicsArchL1AcceptAction::fire():clock_gettime() Failed: %s\n", strerror(iStatus) );               
            return in;
        }
        
        // check if delta time < (_fMinTriggerInterval) second => do nothing
        if ( (tsCurrent.tv_sec-tsPrev.tv_sec) + (tsCurrent.tv_nsec-tsPrev.tv_nsec)/1.0e9f < _fMinTriggerInterval )
            return in;    
            
        printf( "\n\n===== Writing L1 Data =====\n" );            
        InDatagram* out = new ( _manager.getPool() ) Pds::CDatagram( in->datagram() );
        int iFail = _manager.writeMonitoredContent( out->datagram() );                
        
        /*
         * Possible failure modes for _manager.writeMonitoredContent()
         *
         * 1. Some PV's ctrl value has not been updated
         * 2. Some PV's time value has not been updated, but its ctrl value has been written out previously
         * 3. Memory pool size is not enough for storing PV data
         * 4. Other reasons. cf. EpicsMonitorPv::writeXtc()
         */
        if ( iFail != 0 )
        {
            printf( "EpicsArchL1AcceptAction::fire(): writeMonitoredContent() failed, error code = %d\n", iFail );
            delete out;
            return in;
        }                
                  
        // !! for debug test only
        printf( "\nOutput payload size = %d\n", out->datagram().xtc.sizeofPayload());
        
        tsPrev = tsCurrent;  
        return out;
    }
  
private:        
    EpicsArchManager&   _manager;
    float               _fMinTriggerInterval;
    timespec            tsPrev;
};

class EpicsArchDisableAction : public Action 
{
public:
    EpicsArchDisableAction(EpicsArchManager& manager) : _manager(manager)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
        //printf("EpicsArchManager received %d l1accepts\n",cpol1);
        // Call _mrg.Disable()
        return in;
    }
private:
    EpicsArchManager& _manager;
};

const Src EpicsArchManager::srcLevel = Src(Level::Reporter);

EpicsArchManager::EpicsArchManager(CfgClientNfs& cfg, const std::string& sFnConfig, float fMinTriggerInterval) :
  _sFnConfig(sFnConfig), _fMinTriggerInterval(fMinTriggerInterval),
  _pMonitor(NULL) // _pMonitor need to be initialized in the task thread
{
    _pFsm            = new Fsm();    
    _pActionMap      = new EpicsArchAllocAction(*this, cfg);
    _pActionConfig   = new EpicsArchConfigAction(*this, EpicsArchManager::srcLevel, cfg);  // Level::Reporter for Epics Archiver
    _pActionL1Accept = new EpicsArchL1AcceptAction(*this, _fMinTriggerInterval);
    _pActionDisable  = new EpicsArchDisableAction(*this);
    
    _pFsm->callback(TransitionId::Map,        _pActionMap);
    _pFsm->callback(TransitionId::Configure,  _pActionConfig);
    _pFsm->callback(TransitionId::L1Accept,   _pActionL1Accept);
    _pFsm->callback(TransitionId::Disable,    _pActionDisable);    
    
    _pPool = new GenericPool(EpicsArchMonitor::iMaxXtcSize, 1);
}

EpicsArchManager::~EpicsArchManager()
{
    delete _pPool;     
    delete _pMonitor;
    
    delete _pActionDisable;
    delete _pActionL1Accept;
    delete _pActionConfig;
    delete _pActionMap; 
    
    delete _pFsm;    
}

int EpicsArchManager::onActionMap()
{
    return initMonitor();
}

int EpicsArchManager::initMonitor()
{
    delete _pMonitor;

    try
    {
        _pMonitor = new EpicsArchMonitor( _sFnConfig );    
    }
    catch (string& sError)
    {
        printf( "EpicsArchManager::initMonitor(): new EpicsArchMonitor( %s )failed: %s\n",
            _sFnConfig.c_str(), sError.c_str() );
        _pMonitor = NULL;
        return 1;
    }    
    
    return 0;
}

int EpicsArchManager::writeMonitoredContent( Datagram& dg )
{
    if ( _pMonitor == NULL )
    {
        printf( "EpicsArchManager::writeMonitoredContent(): Epics Monitor has not been init-ed\n" );
        return 100;
    }        

    return _pMonitor->writeToXtc( dg );
}
