#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <new>
#include <errno.h>
#include <math.h>

#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pds/service/GenericPool.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/config/CfgClientNfs.hh"
#include "EpicsArchMonitor.hh"
#include "EpicsArchManager.hh"

using namespace Pds;
using std::string;

//
//  After a call to 'reset()', one set of data goes to each receiving node.
//  This behavior emulates the broadcast of a transition.
//
class DestinationFilter {
public:
    DestinationFilter() : _numberOfDst(0) {}

    void reset() 
    {
        _remainingDst = (1<<_numberOfDst)-1;
    }
  
    bool accept(const InDatagram& in) 
    {
        unsigned dst = 1 << (in.datagram().seq.stamp().vector()%_numberOfDst);
	if ( (_remainingDst & dst) ) 
	{
	    _remainingDst ^= dst;
	    return true;
	}
	return false;
    }

    void map(const Allocation& alloc)
    {
        _numberOfDst = 0;
        for(unsigned i=0; i<alloc.nnodes(); i++)
	    if (alloc.node(i)->level() == Level::Event)
	        _numberOfDst++;
    }  

private:
    unsigned _numberOfDst;
    unsigned _remainingDst;
};

class EpicsArchAllocAction : public Action 
{
public:
  EpicsArchAllocAction(EpicsArchManager& manager, DestinationFilter& filter, CfgClientNfs& cfg) : 
    _manager(manager), _filter(filter), _cfg(cfg) {}
    
    virtual Transition* fire(Transition* tr)     
    {
        const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
        _cfg.initialize(alloc.allocation());  
	_filter.map(alloc.allocation());
        _manager.onActionMap();
        return tr;
    }
private:
    EpicsArchManager&  _manager;
    DestinationFilter& _filter;
    CfgClientNfs&      _cfg;
};

class EpicsArchConfigAction : public Action 
{
public:
    EpicsArchConfigAction(EpicsArchManager& manager, const Src& src, CfgClientNfs& cfg, int iDebugLevel) :
        //_cfgtc(_epicsArchConfigType,src),
      _manager(manager), _src(src), _cfg(cfg), _iDebugLevel(iDebugLevel),
      _occPool(sizeof(UserMessage),1)
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
        if (_iDebugLevel>=1) printf( "\n\n===== Writing Configs =====\n" );

        // insert assumes we have enough space in the input datagram
        //dg->insert(_cfgtc, &_config);
        //if (_nerror) dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        InDatagram* out = new ( _manager.getPool() ) Pds::CDatagram( in->datagram() );
        int iFail = _manager.writeMonitoredContent( out->datagram(), true );
        
        /*
         * Possible failure modes for _manager.writeMonitoredConfigContent()
         *   cf. EpicsArchMonitor::writeToXtc() return codes
         * 
         * Error Code     Reason
         * 
         * 2              Memory pool size is not enough for storing PV data
         * 3              All PV write failed. No PV value is outputted
         * 4              Some PV values have been outputted, but some has write error
         * 5              Some PV values have been outputted, but some has not been connected
         */
	static const char* cfgError[] = { NULL, 
					  NULL,
					  ":PV data size too large", 
					  NULL,
					  NULL,
					  ":Some PVs not connected" };
        if ( iFail != 0 )
        {
          // set damage bit          
          out->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
	  if (cfgError[iFail]) {
	    UserMessage* msg = new(&_occPool) UserMessage;
	    msg->append(DetInfo::name(static_cast<const DetInfo&>(_cfg.src())));
	    msg->append(cfgError[iFail]);
	    _manager.appliance().post(msg);
	  }
        }                
                  
        if (_iDebugLevel>=1) printf( "\nOutput payload size = %d\n", out->datagram().xtc.sizeofPayload());
        
        return out;
    }

private:
    //EpicsArchConfigType _config;
    //Xtc _cfgtc;
    EpicsArchManager&   _manager;
    Src                 _src;
    CfgClientNfs&       _cfg;
    int                 _iDebugLevel;
    GenericPool         _occPool;
};

class EpicsArchL1AcceptAction : public Action 
{
public:
    EpicsArchL1AcceptAction(EpicsArchManager& manager, float fMinTriggerInterval, DestinationFilter* filter, int iDebugLevel) :
        _manager(manager) , _fMinTriggerInterval(fMinTriggerInterval), _filter(filter), _iDebugLevel(iDebugLevel)
    {
        tsPrev.tv_sec = tsPrev.tv_nsec = 0;   
    }
    
    ~EpicsArchL1AcceptAction()
    { delete _filter; }
    
    virtual InDatagram* fire(InDatagram* in)     
    {
        // Check delta time
        timespec tsCurrent;
        int iStatus = clock_gettime (CLOCK_REALTIME, &tsCurrent);
        if (iStatus)
        {
            printf( "EpicsArchL1AcceptAction::fire():clock_gettime() Failed: %s\n", strerror(iStatus) );               
        }
        // check if delta time < (_fMinTriggerInterval) second => do nothing
        else if ( (tsCurrent.tv_sec-tsPrev.tv_sec) + (tsCurrent.tv_nsec-tsPrev.tv_nsec)/1.0e9f >= _fMinTriggerInterval )
	{
	    _filter->reset();
        }
    
	if (!_filter->accept(*in))
	{
  	    return in;
	}

        if (_iDebugLevel >= 1) printf( "\n\n===== Writing L1 Data =====\n" );
        InDatagram* out = new ( _manager.getPool() ) Pds::CDatagram( in->datagram() );
        int iFail = _manager.writeMonitoredContent( out->datagram(), false );                
        
        /*
         * Possible failure modes for _manager.writeMonitoredConfigContent()
         *   cf. EpicsArchMonitor::writeToXtc() return codes
         * 
         * Error Code     Reason
         * 
         * 2              Memory pool size is not enough for storing PV data
         * 3              All PV write failed. No PV value is outputted
         * 4              Some PV values have been outputted, but some has write error
         * 5              Some PV values have been outputted, but some has not been connected
         */
        if ( iFail != 0 )
        {
          // set damage bit
          out->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        }                
                  
        if (_iDebugLevel >= 1) printf( "\nOutput payload size = %d\n", out->datagram().xtc.sizeofPayload());
        
        tsPrev = tsCurrent;  
        return out;
    }

private:        
    EpicsArchManager&   _manager;
    float               _fMinTriggerInterval;
    DestinationFilter*  _filter;
    int                 _iDebugLevel;
    timespec            tsPrev;    
};

class EpicsArchDisableAction : public Action 
{
public:
    EpicsArchDisableAction(EpicsArchManager& manager) : _manager(manager)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
        return in;
    }
private:
    EpicsArchManager& _manager;
};

const Src EpicsArchManager::srcLevel = Src(Level::Reporter);

EpicsArchManager::EpicsArchManager(CfgClientNfs& cfg, const std::string& sFnConfig, float fMinTriggerInterval, int iDebugLevel) :
  _sFnConfig(sFnConfig), _fMinTriggerInterval(fMinTriggerInterval), _iDebugLevel(iDebugLevel),
  _pMonitor(NULL) // _pMonitor need to be initialized in the task thread
{
    DestinationFilter* filter = new DestinationFilter;
    _pFsm            = new Fsm();    
    _pActionMap      = new EpicsArchAllocAction(*this, *filter, cfg);
    _pActionConfig   = new EpicsArchConfigAction(*this, EpicsArchManager::srcLevel, cfg, _iDebugLevel);  // Level::Reporter for Epics Archiver
    _pActionL1Accept = new EpicsArchL1AcceptAction(*this, _fMinTriggerInterval, filter, _iDebugLevel);
    _pActionDisable  = new EpicsArchDisableAction(*this);
    
    _pFsm->callback(TransitionId::Map,        _pActionMap);
    _pFsm->callback(TransitionId::Configure,  _pActionConfig);
    _pFsm->callback(TransitionId::L1Accept,   _pActionL1Accept);
    _pFsm->callback(TransitionId::Disable,    _pActionDisable);    
    
    _pPool = new GenericPool(EpicsArchMonitor::iMaxXtcSize, 8);
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
    // initialize thread-specific data
    return initMonitor();
}

int EpicsArchManager::initMonitor()
{
    delete _pMonitor;

    try
    {
        _pMonitor = new EpicsArchMonitor( _sFnConfig, _iDebugLevel );
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

int EpicsArchManager::writeMonitoredContent( Datagram& dg, bool bCtrlValue )
{
    if ( _pMonitor == NULL )
    {
        printf( "EpicsArchManager::writeMonitoredConfigContent(): Epics Monitor has not been init-ed\n" );
        return 100;
    }        

    return _pMonitor->writeToXtc( dg, bCtrlValue );
}

