#include "PrincetonManager.hh"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>
#include <sys/mman.h>
#include <new>
#include <vector>

#include "pdsdata/princeton/FrameV1.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/client/Action.hh"
#include "pds/client/Response.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/utility/StreamPorts.hh"
#include "PrincetonServer.hh"

using std::string;

namespace Pds 
{

static int printDataTime(const InDatagram* in);

class PrincetonMapAction : public Action 
{
public:
    PrincetonMapAction(PrincetonManager& manager, CfgClientNfs& cfg, int iDebugLevel) : 
      _manager(manager), _cfg(cfg), _iMapCameraFail(0), _iDebugLevel(iDebugLevel) {}
    
    virtual Transition* fire(Transition* tr)     
    {
        const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
        _cfg.initialize(alloc.allocation());  
        
        _iMapCameraFail = _manager.mapCamera(alloc.allocation());
        return tr;                
    }
    
    virtual InDatagram* fire(InDatagram* in) 
    {
      if (_iDebugLevel >= 1) 
        printf( "\n\n===== Writing Map Data =========\n" );          
      if (_iDebugLevel>=2) printDataTime(in);
      
      if ( _iMapCameraFail != 0 )
        in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);      
      return in;
    }
private:
    PrincetonManager& _manager;
    CfgClientNfs&     _cfg;
    int               _iMapCameraFail;    
    int               _iDebugLevel;
};

class PrincetonConfigAction : public Action 
{
public:
    PrincetonConfigAction(PrincetonManager& manager, CfgClientNfs& cfg, bool bDelayMode, int iDebugLevel) :
        _manager(manager), _cfg(cfg), _bDelayMode(bDelayMode), _iDebugLevel(iDebugLevel),
        _cfgtc(_typePrincetonConfig, cfg.src()), _configCamera(), _iConfigCameraFail(0)
    {}
    
    virtual Transition* fire(Transition* tr) 
    {
      int iConfigSize = _cfg.fetch(*tr, _typePrincetonConfig, &_configCamera, sizeof(_configCamera));
      
      if ( iConfigSize == 0 ) // no config data is found in the database.       
      {
        printf( "PrincetonConfigAction::fire(): No config data is loaded. Will use default values for configuring the camera.\n" );
        _configCamera = Princeton::ConfigV2(
          16, // Width
          16, // Height
          0,  // OrgX
          0,  // OrgX
          1,  // BinX
          1,  // BinX
          0.001,  // Exposure time
          25.0f,  // Cooling temperature
          3,  // Gain index
          1,  // Readout speed index
          1   // Redout event code
        );
      }
              
      if ( iConfigSize != 0 && iConfigSize != sizeof(_configCamera) )
      {
        printf( "PrincetonConfigAction::fire(): Config data has incorrect size (%d B). Should be %d B.\n",
          iConfigSize, sizeof(_configCamera) );
          
        _configCamera       = Princeton::ConfigV2();
        _iConfigCameraFail  = 1;
      }
                  
      _iConfigCameraFail = _manager.configCamera(_configCamera);
      
      return tr;
    }

    virtual InDatagram* fire(InDatagram* in) 
    {
        if (_iDebugLevel>=1) printf("\n\n===== Writing Configs =====\n" );
        if (_iDebugLevel>=2) printDataTime(in);

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
                  
        if (_iDebugLevel>=2) printf( "Princeton Config data:\n"
          "  Width %d Height %d  Org X %d Y %d  Bin X %d Y %d\n"
          "  Exposure time %gs  Cooling Temperature %.1f C  Gain Index %d\n"
          "  Readout Speed %d  Readout Event %d Delay Mode %d\n",
          _configCamera.width(), _configCamera.height(),
          _configCamera.orgX(), _configCamera.orgY(),
          _configCamera.binX(), _configCamera.binY(),
          _configCamera.exposureTime(), _configCamera.coolingTemp(), _configCamera.gainIndex(), 
          _configCamera.readoutSpeedIndex(), _configCamera.readoutEventCode(), ( _configCamera.delayMode() ? 1: 0 )
          );
        if (_iDebugLevel>=1) printf( "\nOutput payload size = %d\n", in->datagram().xtc.sizeofPayload());
        
        return in;
    }

private:
    PrincetonManager&   _manager;    
    CfgClientNfs&       _cfg;
    bool                _bDelayMode;
    const int           _iDebugLevel;
    Xtc                 _cfgtc;
    Princeton::ConfigV2 _configCamera;    
    int                 _iConfigCameraFail;
    
    /*
     * private static consts
     */
    static const TypeId _typePrincetonConfig;    
};

/*
 * Definition of private static consts
 */
const TypeId PrincetonConfigAction::_typePrincetonConfig = TypeId(TypeId::Id_PrincetonConfig, Princeton::ConfigV2::Version);

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
      if (_iDebugLevel >= 1) 
        printf( "\n\n===== Writing Unmap Data =========\n" );          
      if (_iDebugLevel>=2) printDataTime(in);
      
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
      if (_iDebugLevel >= 1) 
        printf( "\n\n===== Writing BeginRun Data =========\n" );          
      if (_iDebugLevel>=2) printDataTime(in);
      
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
      if (_iDebugLevel >= 1) 
        printf( "\n\n===== Writing EndRun Data =========\n" );          
      if (_iDebugLevel>=2) printDataTime(in);
      
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
    PrincetonL1AcceptAction(PrincetonManager& manager, CfgClientNfs& cfg, bool bDelayMode, int iDebugLevel) :
        _manager(manager), _cfg(cfg), _bDelayMode(bDelayMode), _iDebugLevel(iDebugLevel)
        //, _poolFrameData(1024*1024*8 + 1024, 16) // pool for debug
    {
    }
            
    virtual InDatagram* fire(InDatagram* in)     
    {               
      int         iFail = 0;
      InDatagram* out   = in;
      
/*!! recover from delay mode
      int   iShotId     = in->datagram().seq.stamp().fiducials();   // shot ID 
                        
      iFail = _manager.checkReadoutEventCode(in);
      //iFail = 0; // !! debug
      
      // Discard the evr data, for appending the detector data later
      in->datagram().xtc.extent = sizeof(Xtc);            
      
      if ( iFail != 0 )
      {
        if (_iDebugLevel >= 1) 
          printf( "No readout event code\n" );
          
        return in;
      }      
      
      if ( _bDelayMode )
      {
        iFail =  _manager.getDelayData( in, out );        
        iFail |= _manager.onEventReadoutDelay( iShotId, in );
      }
      else
      { // prompt mode
        iFail = _manager.onEventReadoutPrompt( iShotId, in, out );          
      }        
*/      
      iFail =  _manager.getDelayData( in, out );        
                    
      if ( iFail != 0 )
      {
        // set damage bit
        out->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
      }              
      
      /*
       * The folloing code is used for debugging variable L1 data size
       */
      //int iDataSize = 1024*1024*8 + sizeof(Princeton::FrameV1);            
      //out = 
      // new ( &_poolFrameData ) CDatagram( in->datagram() ); 
      //out->datagram().xtc.alloc( sizeof(Xtc) + iDataSize );        
      //unsigned char* pXtcHeader = (unsigned char*) out + sizeof(CDatagram);
      //   
      //TypeId typePrincetonFrame(TypeId::Id_PrincetonFrame, Princeton::FrameV1::Version);
      //Xtc* pXtcFrame = 
      // new ((char*)pXtcHeader) Xtc(typePrincetonFrame, _cfg.src() );
      //pXtcFrame->alloc( iDataSize );      
                
      if (_iDebugLevel >= 1 && out != in) 
      {
        printf( "\n\n===== Writing L1Accept Data =========\n" );          
        if (_iDebugLevel>=2) printDataTime(in);
        
        if (_iDebugLevel >= 3) 
        {
          Xtc& xtcData = in->datagram().xtc;
          printf( "\nInput payload size = %d\n", xtcData.sizeofPayload() );
        }
        
        Xtc& xtcData = out->datagram().xtc;
        printf( "\nOutput payload size = %d  fail = %d\n", xtcData.sizeofPayload(), iFail);
        Xtc& xtcFrame = *(Xtc*) xtcData.payload();
        
        if ( xtcData.sizeofPayload() != 0 ) 
        {
          printf( "Frame  payload size = %d\n", xtcFrame.sizeofPayload());
          Princeton::FrameV1& frameData = *(Princeton::FrameV1*) xtcFrame.payload();
          printf( "Frame  Id 0x%05x  ReadoutTime %.2fs\n", frameData.shotIdStart(), 
           frameData.readoutTime() );
        }
      }

      if ( out == in && !_bDelayMode )
      {
        printf("\r");
        fflush(NULL);
        printf("\r");
        fflush(NULL);
        //timeval timeSleepMicro = {0, 500}; // 0.5 ms
        // Use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
        //select( 0, NULL, NULL, NULL, &timeSleepMicro);
      }
      
      return out;
    }
  
private:        
    PrincetonManager&   _manager;
    CfgClientNfs&       _cfg;
    bool                _bDelayMode;    
    int                 _iDebugLevel;
    //GenericPool         _poolFrameData; // pool for debug
};


class PrincetonEnableAction : public Action 
{
public:
    PrincetonEnableAction(PrincetonManager& manager, int iDebugLevel) : 
     _manager(manager), _iDebugLevel(iDebugLevel),
     _iEnableCameraFail(0)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
      _iEnableCameraFail = _manager.enableCamera();
      return in;
    }
    
    virtual InDatagram* fire(InDatagram* in)     
    {
      if ( _iEnableCameraFail != 0 )
        in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        
      if (_iDebugLevel >= 1) 
        printf( "\n\n===== Writing Enable Data =========\n" );
      if (_iDebugLevel>=2) printDataTime(in);
        
      return in;
    }
    
private:
    PrincetonManager& _manager;
    int               _iDebugLevel;
    int               _iEnableCameraFail;
};

class PrincetonDisableAction : public Action 
{
public:
    PrincetonDisableAction(PrincetonManager& manager, int iDebugLevel) : 
     _manager(manager), _occPool(sizeof(UserMessage),2), _iDebugLevel(iDebugLevel), 
     _iDisableCameraFail(0)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
      return in;
    }
    
    virtual InDatagram* fire(InDatagram* in)     
    {
      if (_iDebugLevel >= 1) 
        printf( "\n\n===== Writing Disable Data =========\n" );          
      if (_iDebugLevel>=2) printDataTime(in);
        
      InDatagram* out = in;
      
      // In either normal mode or delay mode, we will need to check if there is
      // an un-reported data
      int iFail = _manager.getLastDelayData( in, out );

      if ( iFail != 0 )
        out->datagram().xtc.damage.increase(Pds::Damage::UserDefined); // set damage bit            
        
      if (_iDebugLevel >= 1) 
      {
        Xtc& xtcData = out->datagram().xtc;
        printf( "\nOutput payload size = %d  fail = %d\n", xtcData.sizeofPayload(), iFail);
        Xtc& xtcFrame = *(Xtc*) xtcData.payload();
        
        if ( xtcData.sizeofPayload() != 0 ) 
        {
          printf( "Frame  payload size = %d\n", xtcFrame.sizeofPayload());
          Princeton::FrameV1& frameData = *(Princeton::FrameV1*) xtcFrame.payload();
          printf( "Frame Id Start %d ReadoutTime %f\n", frameData.shotIdStart(), 
           frameData.readoutTime() );
        }
      }          
      
      _iDisableCameraFail = _manager.disableCamera();
      if ( _iDisableCameraFail != 0 )
        in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
      
    if (out != in)
    {
      UserMessage* msg = new(&_occPool) UserMessage();
      msg->append("Run stopped before princeton data is sent out,\n");
      msg->append("so the data is attached to the Disable event.");      
      _manager.appliance().post(msg);
    }
      
      return out;
    }
    
private:
    PrincetonManager& _manager;
    GenericPool       _occPool;
    int               _iDebugLevel;
    int               _iDisableCameraFail;    
};

//
//  **weaver
//
class PrincetonResponse : public Response {
public:
  PrincetonResponse(PrincetonManager& mgr, int iDebugLevel) :
    _manager(mgr), _iDebugLevel(iDebugLevel)
  {
  }
public:
  Occurrence* fire(Occurrence* occ) {
    if (_iDebugLevel >= 1) 
      printf( "\n\n===== Get Occurance =========\n" );          
    if (_iDebugLevel>=2) printDataTime(NULL);

    const EvrCommand& cmd = *reinterpret_cast<const EvrCommand*>(occ);
    if (_manager.checkReadoutEventCode(cmd.code)) {
      if (_iDebugLevel >= 1) 
        printf( "Get command event code: %u\n", cmd.code );          

      _manager.onEventReadout();
    }
    return 0;
  }
private:
  PrincetonManager& _manager;
  int               _iDebugLevel;
};

PrincetonManager::PrincetonManager(CfgClientNfs& cfg, int iCamera, bool bDelayMode, bool bInitTest, 
  string sConfigDb, int iSleepInt, int iDebugLevel) :
  _iCamera(iCamera), _bDelayMode(bDelayMode), _bInitTest(bInitTest), 
  _sConfigDb(sConfigDb), _iSleepInt(iSleepInt),
  _iDebugLevel(iDebugLevel), _pServer(NULL)
{
    _pActionMap       = new PrincetonMapAction      (*this, cfg, _iDebugLevel);
    _pActionConfig    = new PrincetonConfigAction   (*this, cfg, _bDelayMode, _iDebugLevel);
    _pActionUnconfig  = new PrincetonUnconfigAction (*this, _iDebugLevel);  
    _pActionBeginRun  = new PrincetonBeginRunAction (*this, _iDebugLevel);
    _pActionEndRun    = new PrincetonEndRunAction   (*this, _iDebugLevel);  
    _pActionEnable    = new PrincetonEnableAction   (*this, _iDebugLevel);
    _pActionDisable   = new PrincetonDisableAction  (*this, _iDebugLevel);
    _pActionL1Accept  = new PrincetonL1AcceptAction (*this, cfg, _bDelayMode, _iDebugLevel);
    _pResponse        = new PrincetonResponse       (*this, _iDebugLevel);

    try
    {     
    _pServer = new PrincetonServer(_iCamera, _bDelayMode, _bInitTest, cfg.src(), _sConfigDb, _iSleepInt, _iDebugLevel);    
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
    _pFsm->callback(TransitionId::Enable,       _pActionEnable);            
    _pFsm->callback(TransitionId::Disable,      _pActionDisable);            
    _pFsm->callback(TransitionId::L1Accept,     _pActionL1Accept);
    _pFsm->callback(OccurrenceId::EvrCommand,   _pResponse);
}

PrincetonManager::~PrincetonManager()
{   
    delete _pFsm;
    
    delete _pServer;
    
    delete _pResponse;
    delete _pActionL1Accept;
    delete _pActionDisable;
    delete _pActionEnable;
    delete _pActionEndRun;
    delete _pActionBeginRun;
    delete _pActionUnconfig;    
    delete _pActionConfig;
    delete _pActionMap; 
}

int PrincetonManager::mapCamera(const Allocation& alloc)
{
  return _pServer->mapCamera();
}

int PrincetonManager::configCamera(Princeton::ConfigV2& config)
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

int PrincetonManager::enableCamera()
{
  return _pServer->enableCamera();
}

int PrincetonManager::disableCamera()
{
  return _pServer->disableCamera();
}

int PrincetonManager::onEventReadout()
{
  return _pServer->onEventReadout();
}

/* !! recover from delay mode
int PrincetonManager::onEventReadoutPrompt(int iShotId, InDatagram* in, InDatagram*& out)
{
  return _pServer->onEventReadoutPrompt(iShotId, in, out);  
}

int PrincetonManager::onEventReadoutDelay(int iShotId, InDatagram* in)
{
  return _pServer->onEventReadoutDelay(iShotId, in);  
}
*/

int PrincetonManager::getDelayData(InDatagram* in, InDatagram*& out)
{
  return _pServer->getDelayData(in, out);
}

int PrincetonManager::getLastDelayData(InDatagram* in, InDatagram*& out)
{
  return _pServer->getLastDelayData(in, out);
}

int PrincetonManager::checkReadoutEventCode(unsigned code)
{
  return _pServer->checkReadoutEventCode(code);
}

/* 
 * Print the local timestamp and the data timestamp
 *
 * If the input parameter in == NULL, no data timestamp will be printed
 */
static int printDataTime(const InDatagram* in)
{
  static const char sTimeFormat[40] = "%02d_%02H:%02M:%02S"; /* Time format string */    
  char sTimeText[40];
  
  time_t timeCurrent;
  time(&timeCurrent);          
  strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeCurrent));
  
  printf("Local Time: %s\n", sTimeText);

  if ( in == NULL ) return 0;
  
  const ClockTime clockCurDatagram  = in->datagram().seq.clock();
  uint32_t        uFiducial         = in->datagram().seq.stamp().fiducials();
  timeCurrent                       = clockCurDatagram.seconds();
  strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeCurrent));
  printf("Data  Time: %s  Fiducial: 0x%05x\n", sTimeText, uFiducial);
  return 0;
}

} //namespace Pds 
