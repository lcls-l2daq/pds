#include "PrincetonManager.hh"

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
#include "pds/client/Action.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/princeton/PrincetonServer.hh"

using std::string;

namespace Pds 
{
  
using namespace Princeton;

class PrincetonAllocAction : public Action 
{
public:
    PrincetonAllocAction(PrincetonManager& manager, CfgClientNfs& cfg) : _manager(manager), _cfg(cfg) {}
    
    virtual Transition* fire(Transition* tr)     
    {
        const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
        _cfg.initialize(alloc.allocation());  
        
        return tr;                
    }
private:
    PrincetonManager& _manager;
    CfgClientNfs& _cfg;
};

class PrincetonConfigAction : public Action 
{
public:
    PrincetonConfigAction(PrincetonManager& manager, CfgClientNfs& cfg, int iDebugLevel) :
        _manager(manager), _cfg(cfg), _iDebugLevel(iDebugLevel),
        _cfgtc(_typePrincetonConfig, cfg.src()), _configCamera(), _iConfigCameraFail(0)
    {}
    
    // this is the first "phase" of a transition where
    // all CPUs configure in parallel.
    virtual Transition* fire(Transition* tr) 
    {
      _cfg.fetch(*tr, _typePrincetonConfig, &_configCamera, sizeof(_configCamera));
      return tr; // !! for debugging only
      
      _iConfigCameraFail = _manager.configCamera(_configCamera);
      
      return tr;
    }

    // this is the second "phase" of a transition where everybody
    // records the results of configure (which will typically be
    // archived in the xtc file).
    virtual InDatagram* fire(InDatagram* in) 
    {
        if (_iDebugLevel>=1) printf( "\n\n===== Writing Configs =====\n" );

        // insert assumes we have enough space in the memory pool for in datagram
        _cfgtc.alloc( sizeof(_configCamera) );
        in->insert(_cfgtc, &_configCamera);
        
        if ( _iConfigCameraFail  != 0 )
          in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
                  
        if (_iDebugLevel>=1) printf( "\nOutput payload size = %d\n", in->datagram().xtc.sizeofPayload());
        
        return in;
    }

private:
    PrincetonManager&   _manager;
    CfgClientNfs&       _cfg;
    const int           _iDebugLevel;
    Xtc                 _cfgtc;
    Princeton::ConfigV1 _configCamera;    
    int                 _iConfigCameraFail;
    
    /*
     * private static consts
     */
    static const TypeId _typePrincetonConfig;    
};

/*
 * Definition of private static consts
 */
const TypeId PrincetonConfigAction::_typePrincetonConfig = TypeId(TypeId::Id_PrincetonConfig, Princeton::ConfigV1::Version);

class PrincetonUnconfigAction : public Action 
{
public:
    PrincetonUnconfigAction(PrincetonManager& manager, int iDebugLevel) : _manager(manager), _iDebugLevel(iDebugLevel)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
      _manager.unconfigCamera();
      return in;
    }
private:
    PrincetonManager& _manager;
    int               _iDebugLevel;
};

class PrincetonL1AcceptAction : public Action 
{
public:
    PrincetonL1AcceptAction(PrincetonManager& manager, bool bMakeUpEvent, int iDebugLevel) :
        _manager(manager), _bMakeUpEvent(bMakeUpEvent), _iDebugLevel(iDebugLevel)
    {
    }
    
    ~PrincetonL1AcceptAction()
    {}
    
    virtual InDatagram* fire(InDatagram* in)     
    {            
        if (_iDebugLevel >= 1) printf( "\n\n===== Writing L1 Data (Stream Mode) =====\n" );
        
        int   iShotId       = 1;      // !! Obtain shot ID 
        bool  bCaptureStart = false;  // !! Check if this event is for starting capture
        bool  bCaptureEnd   = false;  // !! Check if this event is for stoping capture        
        
        return in; // !! for debugging only
        if ( _bMakeUpEvent )
        {
          InDatagram* out = in;
          int iFail = _manager.getMakeUpData( in, out );

          if ( iFail != 0 )
            out->datagram().xtc.damage.increase(Pds::Damage::UserDefined); // set damage bit            
        }
        
        if ( bCaptureStart )
        {
          int iFail = _manager.captureStart( iShotId );

          if ( iFail != 0 )
            in->datagram().xtc.damage.increase(Pds::Damage::UserDefined); // set damage bit
        }
        
        if ( !bCaptureEnd )
          return in; // Return empty data        
        
        InDatagram* out = in;
        int iFail = _manager.captureEnd( iShotId, in, out );
        
        /*
         * Possible failure modes for _manager.writeMonitoredConfigContent()
         *   cf. ......
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
        
        return out;
    }
  
private:        
    PrincetonManager&   _manager;
    bool                _bMakeUpEvent;
    int                 _iDebugLevel;
};

class PrincetonDisableAction : public Action 
{
public:
    PrincetonDisableAction(PrincetonManager& manager, int iDebugLevel) : _manager(manager), _iDebugLevel(iDebugLevel)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
        return in;
    }
private:
    PrincetonManager& _manager;
    int               _iDebugLevel;
};

PrincetonManager::PrincetonManager(CfgClientNfs& cfg, bool bMakeUpEvent, const string& sFnOutput, int iDebugLevel) :
  _bMakeUpEvent(bMakeUpEvent), _bStreamMode(sFnOutput.empty()), // If no output filename is specified, then use stream mode
  _iDebugLevel(iDebugLevel)
{
    _pActionMap      = new PrincetonAllocAction(*this, cfg);
    _pActionConfig   = new PrincetonConfigAction(*this, cfg, _iDebugLevel);
    _pActionUnconfig = new PrincetonUnconfigAction(*this, _iDebugLevel);  
    _pActionDisable  = new PrincetonDisableAction(*this, _iDebugLevel);
    _pActionL1Accept = new PrincetonL1AcceptAction(*this, _bMakeUpEvent, _iDebugLevel);
                   
    /*
     * Determine the polling scheme
     *
     * 1. In normal mode, use L1 Accept event handler to do the polling -> bUseCaptureThread = false
     * 2. In make-up mode, use camera thread to do the polling          -> bUseCaptureThread = true
     */  
    bool bUseCaptureThread = _bMakeUpEvent; 

    try
    {
      
    _pServer = new PrincetonServer(bUseCaptureThread, _bStreamMode, sFnOutput, cfg.src(), _iDebugLevel);
    
    }
    catch ( PrincetonServerException& eServer )
    {
      throw PrincetonManagerException( "PrincetonManager::PrincetonManager(): Server Initialization Failed" );
    }
 
    _pFsm = new Fsm();    
    _pFsm->callback(TransitionId::Map,          _pActionMap);
    _pFsm->callback(TransitionId::Configure,    _pActionConfig);
    _pFsm->callback(TransitionId::Unconfigure,  _pActionUnconfig);
    _pFsm->callback(TransitionId::L1Accept,     _pActionL1Accept);
    _pFsm->callback(TransitionId::Disable,      _pActionDisable);        
}

PrincetonManager::~PrincetonManager()
{   
    delete _pFsm;
    
    delete _pServer;
    
    delete _pActionDisable;
    delete _pActionL1Accept;
    delete _pActionConfig;
    delete _pActionMap; 
}

int PrincetonManager::configCamera(Princeton::ConfigV1& config)
{
  return _pServer->configCamera(config);
}

int PrincetonManager::unconfigCamera()
{
  return _pServer->unconfigCamera();
}

int PrincetonManager::captureStart(int iShotIdStart)
{
  return _pServer->captureStart(iShotIdStart);
}

int PrincetonManager::captureEnd(int iShotIdEnd, InDatagram* in, InDatagram*& out)
{
  return _pServer->captureEnd(iShotIdEnd, in, out);
}

int PrincetonManager::getMakeUpData(InDatagram* in, InDatagram*& out)
{
  return _pServer->getMakeUpData(in, out);
}

} //namespace Pds 
