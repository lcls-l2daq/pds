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
#include "pds/xtc/CDatagram.hh"
#include "pds/client/Action.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/princeton/PrincetonServer.hh"

using std::string;

namespace Pds 
{
  
using namespace Princeton;

class PrincetonMapAction : public Action 
{
public:
    PrincetonMapAction(PrincetonManager& manager, CfgClientNfs& cfg) : _manager(manager), _cfg(cfg), _iMapCameraFail(0) {}
    
    virtual Transition* fire(Transition* tr)     
    {
        const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
        _cfg.initialize(alloc.allocation());  
        
        _iMapCameraFail = _manager.mapCamera();
        return tr;                
    }
    
    virtual InDatagram* fire(InDatagram* in) 
    {
      if ( _iMapCameraFail != 0 )
        in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);      
      return in;
    }
private:
    PrincetonManager& _manager;
    CfgClientNfs&     _cfg;
    int               _iMapCameraFail;    
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
      int iConfigSize = _cfg.fetch(*tr, _typePrincetonConfig, &_configCamera, sizeof(_configCamera));
      if ( iConfigSize == 0 ) // No config data found in the database
      {
        // !! do nothing and uses the default settings
        
        //_iConfigCameraFail = 1;
        //return tr;
      }
            
      // !! for debug only
      _iConfigCameraFail = _manager.configCamera(_configCamera);
      
      return tr;
    }

    // this is the second "phase" of a transition where everybody
    // records the results of configure (which will typically be
    // archived in the xtc file).
    virtual InDatagram* fire(InDatagram* in) 
    {
        if (_iDebugLevel>=1) printf( "\n\n===== Writing Configs =====\n" );

        static bool bConfigAllocated = false;        
        if ( !bConfigAllocated )
        {
          // insert assumes we have enough space in the memory pool for in datagram
          _cfgtc.alloc( sizeof(_configCamera) );
          bConfigAllocated = true;
        }
        
        in->insert(_cfgtc, &_configCamera);
        
        if ( _iConfigCameraFail != 0 )
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
    PrincetonUnconfigAction(PrincetonManager& manager, int iDebugLevel) : _manager(manager), _iDebugLevel(iDebugLevel), 
     _iUnConfigCameraFail(0)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
      _iUnConfigCameraFail = _manager.unconfigCamera();
      return in;
    }
    
    virtual InDatagram* fire(InDatagram* in) 
    {
      if ( _iUnConfigCameraFail != 0 )
        in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);      
      return in;
    }    
private:
    PrincetonManager& _manager;
    int               _iDebugLevel;
    int               _iUnConfigCameraFail;
};

class PrincetonBeginRunAction : public Action 
{
public:
    PrincetonBeginRunAction(PrincetonManager& manager, int iDebugLevel) : _manager(manager), _iDebugLevel(iDebugLevel),
     _iBeginRunCameraFail(0)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
      _iBeginRunCameraFail = _manager.beginRunCamera();
      return in;
    }

    virtual InDatagram* fire(InDatagram* in) 
    {
      if ( _iBeginRunCameraFail != 0 )
        in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);      
      return in;
    }    
private:
    PrincetonManager& _manager;
    int               _iDebugLevel;
    int               _iBeginRunCameraFail;
};

class PrincetonEndRunAction : public Action 
{
public:
    PrincetonEndRunAction(PrincetonManager& manager, int iDebugLevel) : _manager(manager), _iDebugLevel(iDebugLevel),
     _iEndRunCameraFail(0)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
      _iEndRunCameraFail = _manager.endRunCamera();
      return in;
    }
    
    virtual InDatagram* fire(InDatagram* in) 
    {
      if ( _iEndRunCameraFail != 0 )
        in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
      return in;
    }        
private:
    PrincetonManager& _manager;
    int               _iDebugLevel;
    int               _iEndRunCameraFail;
};

class PrincetonL1AcceptAction : public Action 
{
public:
    PrincetonL1AcceptAction(PrincetonManager& manager, bool bDelayMode, int iDebugLevel) :
        _manager(manager), _bDelayMode(bDelayMode), _iDebugLevel(iDebugLevel)
    {
    }
    
    ~PrincetonL1AcceptAction()
    {}
        
    virtual InDatagram* fire(InDatagram* in)     
    {       
        if (_iDebugLevel >= 1) printf( "\n\n===== Writing L1 Data (Stream Mode) =====\n" );
        
        int   iShotId       = 12;      // !! Obtain shot ID 
        bool  bReadoutEvent  = true;  // !! For end-event only mode (no capture start event)
        
        InDatagram* out = in;        
        if ( _bDelayMode )
        {
          int iFail = _manager.getDelayData( in, out );

          if ( iFail != 0 )
            out->datagram().xtc.damage.increase(Pds::Damage::UserDefined); // set damage bit            
        }
                
        if ( !bReadoutEvent )
          return out; // Return empty data        
        
        in = out; // Use the output from the above commands as the input datagram for further processing
        int iFail = 0;                 
        if ( bReadoutEvent )
        {
          if ( _bDelayMode )
            iFail = _manager.onEventReadoutPrompt( iShotId, in, out );
          else
            iFail = _manager.onEventReadoutDelay( iShotId );
        }
        
        /*
         * Possible failure modes for _manager.writeMonitoredConfigContent()
         *   cf. ......
         * 
         * Error Code     Reason
         * 
         */
        if ( iFail != 0 )
        {
          // set damage bit
          out->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        }                
                  
        if (_iDebugLevel >= 1) 
        {
          Xtc& xtcData = out->datagram().xtc;
          printf( "\nOutput payload size = %d  fail = %d\n", xtcData.sizeofPayload(), iFail);
          Xtc& xtcFrame = *(Xtc*) xtcData.payload();
          
          if ( iFail == 0 ) 
          {
            printf( "Frame payload size = %d\n", xtcFrame.sizeofPayload());
            FrameV1& frameData = *(FrameV1*) xtcFrame.payload();
            printf( "Frame Id Start %d End %d ReadoutTime %f\n", frameData.shotIdStart(), 
             frameData.shotIdEnd(), frameData.readoutTime() );
          }
        }
        
        return out;
    }
  
private:        
    PrincetonManager&   _manager;
    bool                _bDelayMode;
    int                 _iDebugLevel;
};

class PrincetonDisableAction : public Action 
{
public:
    PrincetonDisableAction(PrincetonManager& manager, bool bDelayMode, int iDebugLevel) : 
     _manager(manager), _bDelayMode(bDelayMode), _iDebugLevel(iDebugLevel)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
        return in;
    }
    
    virtual InDatagram* fire(InDatagram* in)     
    {
      InDatagram* out = in;
      if ( _bDelayMode )
      {
        int iFail = _manager.getLastDelayData( in, out );

        if ( iFail != 0 )
          out->datagram().xtc.damage.increase(Pds::Damage::UserDefined); // set damage bit            
      }      
      return out;
    }
    
private:
    PrincetonManager& _manager;
    bool              _bDelayMode;
    int               _iDebugLevel;
};

PrincetonManager::PrincetonManager(CfgClientNfs& cfg, bool bDelayMode, const string& sFnOutput, int iDebugLevel) :
  _bDelayMode(bDelayMode), _bStreamMode(sFnOutput.empty()), // If no output filename is specified, then use stream mode
  _iDebugLevel(iDebugLevel), _pServer(NULL)
{
    _pActionMap       = new PrincetonMapAction      (*this, cfg);
    _pActionConfig    = new PrincetonConfigAction   (*this, cfg, _iDebugLevel);
    _pActionUnconfig  = new PrincetonUnconfigAction (*this, _iDebugLevel);  
    _pActionBeginRun  = new PrincetonBeginRunAction (*this, _iDebugLevel);
    _pActionEndRun    = new PrincetonEndRunAction   (*this, _iDebugLevel);  
    _pActionDisable   = new PrincetonDisableAction  (*this, _bDelayMode, _iDebugLevel);
    _pActionL1Accept  = new PrincetonL1AcceptAction (*this, _bDelayMode, _iDebugLevel);
                   
    /*
     * Determine the polling scheme
     *
     * 1. In normal mode, use L1 Accept event handler to do the polling -> bUseCaptureThread = false
     * 2. In make-up mode, use camera thread to do the polling          -> bUseCaptureThread = true
     */  
    bool bUseCaptureThread = _bDelayMode; 

    try
    {
      
    // !! for debug only
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
    _pFsm->callback(TransitionId::BeginRun,     _pActionBeginRun);
    _pFsm->callback(TransitionId::EndRun,       _pActionEndRun);
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

int PrincetonManager::mapCamera()
{
  return _pServer->mapCamera();
}

int PrincetonManager::configCamera(Princeton::ConfigV1& config)
{
  return _pServer->configCamera(config);
}

int PrincetonManager::unconfigCamera()
{
  return _pServer->unconfigCamera();
}

int PrincetonManager::beginRunCamera()
{
  return _pServer->beginRunCamera();
}

int PrincetonManager::endRunCamera()
{
  return _pServer->endRunCamera();
}

int PrincetonManager::onEventReadoutPrompt(int iShotId, InDatagram* in, InDatagram*& out)
{
  return _pServer->onEventReadoutPrompt(iShotId, in, out);  
}

int PrincetonManager::onEventReadoutDelay(int iShotId)
{
  return _pServer->onEventReadoutDelay(iShotId);  
}

int PrincetonManager::getDelayData(InDatagram* in, InDatagram*& out)
{
  return _pServer->getDelayData(in, out);
}

int PrincetonManager::getLastDelayData(InDatagram* in, InDatagram*& out)
{
  return _pServer->getLastDelayData(in, out);
}

} //namespace Pds 
