#include "FliManager.hh"

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

#include "pdsdata/fli/FrameV1.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/client/Action.hh"
#include "pds/client/Response.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/utility/StreamPorts.hh"
#include "FliServer.hh"

using std::string;

namespace Pds 
{

static int printDataTime(const InDatagram* in);

class FliMapAction : public Action 
{
public:
    FliMapAction(FliManager& manager, CfgClientNfs& cfg, int iDebugLevel) : 
      _manager(manager), _cfg(cfg), _iMapCameraFail(0), _iDebugLevel(iDebugLevel) {}
    
    virtual Transition* fire(Transition* tr)     
    {
        const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
        _cfg.initialize(alloc.allocation());  
        
        _iMapCameraFail = _manager.map(alloc.allocation());
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
    FliManager& _manager;
    CfgClientNfs&     _cfg;
    int               _iMapCameraFail;    
    int               _iDebugLevel;
};

class FliConfigAction : public Action 
{
public:
    FliConfigAction(FliManager& manager, CfgClientNfs& cfg, bool bDelayMode, int iDebugLevel) :
        _manager(manager), _cfg(cfg), _occPool(sizeof(UserMessage),2), _bDelayMode(bDelayMode), _iDebugLevel(iDebugLevel),
        _cfgtc(_typeFliConfig, cfg.src()), _config(), _iConfigCameraFail(0)
    {}
    
    virtual Transition* fire(Transition* tr) 
    {
      int iConfigSize = _cfg.fetch(*tr, _typeFliConfig, &_config, sizeof(_config));
      
      if ( iConfigSize == 0 ) // no config data is found in the database.       
      {
        printf( "FliConfigAction::fire(): No config data is loaded. Will use default values for configuring the camera.\n" );
        _config = FliConfigType(
          16, // Width
          16, // Height
          0,  // OrgX
          0,  // OrgX
          1,  // BinX
          1,  // BinX
          0.001,  // Exposure time
          25.0f,  // Cooling temperature
          1,  // Gain index
          1,  // Readout speed index
          1   // Redout event code
        );
      }
              
      if ( iConfigSize != 0 && iConfigSize != sizeof(_config) )
      {
        printf( "FliConfigAction::fire(): Config data has incorrect size (%d B). Should be %d B.\n",
          iConfigSize, (int) sizeof(_config) );
          
        _config       = FliConfigType();
        _iConfigCameraFail  = 1;
      }
                  
      string sConfigWarning;
      _iConfigCameraFail = _manager.config(_config, sConfigWarning);
      if (sConfigWarning.size() != 0)
      {
        UserMessage* msg = new(&_occPool) UserMessage();
        msg->append(sConfigWarning.c_str());
        _manager.appliance().post(msg);        
      }      
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
          _cfgtc.alloc( sizeof(_config) );
          bConfigAllocated = true;
        }
        
        in->insert(_cfgtc, &_config);
        
        if ( _iConfigCameraFail != 0 )
          in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
                  
        if (_iDebugLevel>=2) printf( "Fli Config data:\n"
          "  Width %d Height %d  Org X %d Y %d  Bin X %d Y %d\n"
          "  Exposure time %gs  Cooling Temperature %.1f C  Gain Index %d\n"
          "  Readout Speed %d  Readout Event %d Num Delay Shots %d\n",
          _config.width(), _config.height(),
          _config.orgX(), _config.orgY(),
          _config.binX(), _config.binY(),
          _config.exposureTime(), _config.coolingTemp(), _config.gainIndex(), 
          _config.readoutSpeedIndex(), _config.exposureEventCode(), _config.numDelayShots()
          );
        if (_iDebugLevel>=1) printf( "\nOutput payload size = %d\n", in->datagram().xtc.sizeofPayload());
        
        return in;
    }

private:
    FliManager&   _manager;    
    CfgClientNfs&       _cfg;
    GenericPool         _occPool;
    bool                _bDelayMode;
    const int           _iDebugLevel;
    Xtc                 _cfgtc;
    FliConfigType _config;    
    int                 _iConfigCameraFail;
    
    /*
     * private static consts
     */
    static const TypeId _typeFliConfig;    
};

/*
 * Definition of private static consts
 */
const TypeId FliConfigAction::_typeFliConfig = TypeId(TypeId::Id_FliConfig, FliConfigType::Version);

class FliUnconfigAction : public Action 
{
public:
    FliUnconfigAction(FliManager& manager, int iDebugLevel) : _manager(manager), _iDebugLevel(iDebugLevel), 
     _iUnConfigCameraFail(0)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
      _iUnConfigCameraFail = _manager.unconfig();
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
    FliManager& _manager;
    int               _iDebugLevel;
    int               _iUnConfigCameraFail;
};

class FliBeginRunAction : public Action 
{
public:
    FliBeginRunAction(FliManager& manager, int iDebugLevel) : _manager(manager), _iDebugLevel(iDebugLevel),
     _iBeginRunCameraFail(0)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
      _iBeginRunCameraFail = _manager.beginRun();
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
    FliManager& _manager;
    int               _iDebugLevel;
    int               _iBeginRunCameraFail;
};

class FliEndRunAction : public Action 
{
public:
    FliEndRunAction(FliManager& manager, int iDebugLevel) : _manager(manager), _iDebugLevel(iDebugLevel),
     _iEndRunCameraFail(0)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
      _iEndRunCameraFail = _manager.endRun();
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
    FliManager& _manager;
    int               _iDebugLevel;
    int               _iEndRunCameraFail;
};

class FliL1AcceptAction : public Action 
{
public:
    FliL1AcceptAction(FliManager& manager, CfgClientNfs& cfg, bool bDelayMode, int iDebugLevel) :
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
                        
      iFail = _manager.checkExposureEventCode(in);
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
        iFail =  _manager.getData( in, out );        
        iFail |= _manager.startExposureDelay( iShotId, in );
      }
      else
      { // prompt mode
        iFail = _manager.startExposurePrompt( iShotId, in, out );          
      }        
*/      
      bool bWait = false;
      _manager.l1Accept(bWait);
      
      if (bWait)
        iFail =  _manager.waitData( in, out );        
      else
        iFail =  _manager.getData ( in, out );        
                    
      if ( iFail != 0 )
      {
        // set damage bit
        out->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
      }              
      
      /*
       * The folloing code is used for debugging variable L1 data size
       */
      //int iDataSize = 1024*1024*8 + sizeof(Fli::FrameV1);            
      //out = 
      // new ( &_poolFrameData ) CDatagram( in->datagram() ); 
      //out->datagram().xtc.alloc( sizeof(Xtc) + iDataSize );        
      //unsigned char* pXtcHeader = (unsigned char*) out + sizeof(CDatagram);
      //   
      //TypeId typeFliFrame(TypeId::Id_FliFrame, Fli::FrameV1::Version);
      //Xtc* pXtcFrame = 
      // new ((char*)pXtcHeader) Xtc(typeFliFrame, _cfg.src() );
      //pXtcFrame->alloc( iDataSize );      

      //!!debug
      //uint32_t        uVector           = out->datagram().seq.stamp().vector();
      //printf("sending out L1 with vector %d\n", uVector);
          
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
          Fli::FrameV1& frameData = *(Fli::FrameV1*) xtcFrame.payload();
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

        timeval timeSleepMicro = {0, 1000};
      // Use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
        select( 0, NULL, NULL, NULL, &timeSleepMicro);

        //timeval timeSleepMicro = {0, 500}; // 0.5 ms
        // Use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
        //select( 0, NULL, NULL, NULL, &timeSleepMicro);
      }
      
      return out;
    }
  
private:        
    FliManager&   _manager;
    CfgClientNfs&       _cfg;
    bool                _bDelayMode;    
    int                 _iDebugLevel;
    //GenericPool         _poolFrameData; // pool for debug
};

class FliBeginCalibCycleAction : public Action 
{
public:
    FliBeginCalibCycleAction(FliManager& manager, int iDebugLevel) : 
     _manager(manager), _iDebugLevel(iDebugLevel),
     _iBeginCalibCycleCameraFail(0)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
      _iBeginCalibCycleCameraFail = _manager.beginCalibCycle();
      return in;
    }
    
    virtual InDatagram* fire(InDatagram* in)     
    {
      if ( _iBeginCalibCycleCameraFail != 0 )
        in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        
      if (_iDebugLevel >= 1) 
        printf( "\n\n===== Writing BeginCalibCycle Data =========\n" );
      if (_iDebugLevel>=2) printDataTime(in);
        
      return in;
    }
    
private:
    FliManager& _manager;
    int               _iDebugLevel;
    int               _iBeginCalibCycleCameraFail;
};

class FliEndCalibCycleAction : public Action 
{
public:
    FliEndCalibCycleAction(FliManager& manager, int iDebugLevel) : 
     _manager(manager), _iDebugLevel(iDebugLevel),
     _iEndCalibCycleCameraFail(0)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
      _iEndCalibCycleCameraFail = _manager.beginCalibCycle();
      return in;
    }
    
    virtual InDatagram* fire(InDatagram* in)     
    {
      if ( _iEndCalibCycleCameraFail != 0 )
        in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        
      if (_iDebugLevel >= 1) 
        printf( "\n\n===== Writing EndCalibCycle Data =========\n" );
      if (_iDebugLevel>=2) printDataTime(in);
        
      return in;
    }
    
private:
    FliManager& _manager;
    int               _iDebugLevel;
    int               _iEndCalibCycleCameraFail;
};

class FliEnableAction : public Action 
{
public:
    FliEnableAction(FliManager& manager, int iDebugLevel) : 
     _manager(manager), _iDebugLevel(iDebugLevel),
     _iEnableCameraFail(0)
    {}
        
    virtual Transition* fire(Transition* in) 
    {
      _iEnableCameraFail = _manager.enable();
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
    FliManager& _manager;
    int         _iDebugLevel;
    int         _iEnableCameraFail;
};

class FliDisableAction : public Action 
{
public:
    FliDisableAction(FliManager& manager, int iDebugLevel) : 
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
      int iFail = _manager.waitData( in, out );

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
          Fli::FrameV1& frameData = *(Fli::FrameV1*) xtcFrame.payload();
          printf( "Frame Id Start %d ReadoutTime %f\n", frameData.shotIdStart(), 
           frameData.readoutTime() );
        }
      }          
      
      _iDisableCameraFail = _manager.disable();
      if ( _iDisableCameraFail != 0 )
        in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
      
      if (out != in)
      {
        UserMessage* msg = new(&_occPool) UserMessage();
        msg->append("Run stopped before fli data is sent out,\n");
        msg->append("so the data is attached to the Disable event.");      
        _manager.appliance().post(msg);
      }
      
      return out;
    }
    
private:
    FliManager& _manager;
    GenericPool       _occPool;
    int               _iDebugLevel;
    int               _iDisableCameraFail;
};
 
// 
//  **weaver
//
class FliResponse : public Response {
public:
  FliResponse(FliManager& mgr, int iDebugLevel) :
    _manager(mgr), _iDebugLevel(iDebugLevel)
  {
  }
public:
  Occurrence* fire(Occurrence* occ) {
    if (_iDebugLevel >= 1) 
      printf( "\n\n===== Get Occurance =========\n" );          
    if (_iDebugLevel>=2) printDataTime(NULL);

    const EvrCommand& cmd = *reinterpret_cast<const EvrCommand*>(occ);
    if (_manager.checkExposureEventCode(cmd.code)) {
      if (_iDebugLevel >= 1) 
        printf( "Get command event code: %u\n", cmd.code );          

      _manager.startExposure();
    }
    return 0;
  }
private:
  FliManager& _manager;
  int               _iDebugLevel;
};

FliManager::FliManager(CfgClientNfs& cfg, int iCamera, bool bDelayMode, bool bInitTest, 
  string sConfigDb, int iSleepInt, int iDebugLevel) :
  _iCamera(iCamera), _bDelayMode(bDelayMode), _bInitTest(bInitTest), 
  _sConfigDb(sConfigDb), _iSleepInt(iSleepInt),
  _iDebugLevel(iDebugLevel), _pServer(NULL), _uNumShotsInCycle(0)
{
  _pActionMap             = new FliMapAction      (*this, cfg, _iDebugLevel);
  _pActionConfig          = new FliConfigAction   (*this, cfg, _bDelayMode, _iDebugLevel);
  _pActionUnconfig        = new FliUnconfigAction (*this, _iDebugLevel);  
  _pActionBeginRun        = new FliBeginRunAction (*this, _iDebugLevel);
  _pActionEndRun          = new FliEndRunAction   (*this, _iDebugLevel);  
  _pActionBeginCalibCycle = new FliBeginCalibCycleAction 
                                                        (*this, _iDebugLevel);
  _pActionEndCalibCycle   = new FliEndCalibCycleAction   
                                                        (*this, _iDebugLevel);  
  _pActionEnable          = new FliEnableAction   (*this, _iDebugLevel);
  _pActionDisable         = new FliDisableAction  (*this, _iDebugLevel);
  _pActionL1Accept        = new FliL1AcceptAction (*this, cfg, _bDelayMode, _iDebugLevel);
  _pResponse              = new FliResponse       (*this, _iDebugLevel);

  try
  {     
  _pServer = new FliServer(_iCamera, _bDelayMode, _bInitTest, cfg.src(), _sConfigDb, _iSleepInt, _iDebugLevel);    
  }
  catch ( FliServerException& eServer )
  {
    throw FliManagerException( "FliManager::FliManager(): Server Initialization Failed" );
  }

  _pFsm = new Fsm();    
  _pFsm->callback(TransitionId::Map,              _pActionMap);
  _pFsm->callback(TransitionId::Configure,        _pActionConfig);
  _pFsm->callback(TransitionId::Unconfigure,      _pActionUnconfig);
  _pFsm->callback(TransitionId::BeginRun,         _pActionBeginRun);
  _pFsm->callback(TransitionId::EndRun,           _pActionEndRun);
  _pFsm->callback(TransitionId::BeginCalibCycle,  _pActionBeginCalibCycle);
  _pFsm->callback(TransitionId::EndCalibCycle,    _pActionEndCalibCycle);    
  _pFsm->callback(TransitionId::Enable,           _pActionEnable);            
  _pFsm->callback(TransitionId::Disable,          _pActionDisable);            
  _pFsm->callback(TransitionId::L1Accept,         _pActionL1Accept);
  _pFsm->callback(OccurrenceId::EvrCommand,       _pResponse);
}

FliManager::~FliManager()
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

int FliManager::map(const Allocation& alloc)
{
  return _pServer->map();
}

int FliManager::config(FliConfigType& config, std::string& sConfigWarning)
{
  if ( !_bDelayMode )
    config.setNumDelayShots(0);
  
  return _pServer->config(config, sConfigWarning);
}

int FliManager::unconfig()
{
  return _pServer->unconfig();
}

int FliManager::beginRun()
{
  return _pServer->beginRun();
}

int FliManager::endRun()
{
  return _pServer->endRun();
}

int FliManager::beginCalibCycle()
{
  _uNumShotsInCycle = 0;
  return _pServer->beginCalibCycle();
}

int FliManager::endCalibCycle()
{
  return _pServer->endCalibCycle();
}

int FliManager::enable()
{
  return _pServer->enable();
}

int FliManager::disable()
{
  return _pServer->disable();
}

int FliManager::l1Accept(bool& bWait)
{ 
  ++_uNumShotsInCycle;
  
  if (_bDelayMode)
    bWait = false;
  else
    bWait = (_uNumShotsInCycle>= _pServer->config().numDelayShots());
  return 0;
}

int FliManager::startExposure()
{
  return _pServer->startExposure();
}

/* !! recover from delay mode
int FliManager::startExposurePrompt(int iShotId, InDatagram* in, InDatagram*& out)
{
  return _pServer->startExposurePrompt(iShotId, in, out);  
}

int FliManager::startExposureDelay(int iShotId, InDatagram* in)
{
  return _pServer->startExposureDelay(iShotId, in);  
}
*/

int FliManager::getData(InDatagram* in, InDatagram*& out)
{
  return _pServer->getData(in, out);
}

int FliManager::waitData(InDatagram* in, InDatagram*& out)
{
  int iFail = _pServer->waitData(in, out);
  
  if (out != in)
    _uNumShotsInCycle = 0;
    
  return iFail;
}

int FliManager::checkExposureEventCode(unsigned code)
{
  return (code == _pServer->config().exposureEventCode());
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
  uint32_t        uVector           = in->datagram().seq.stamp().vector();
  timeCurrent                       = clockCurDatagram.seconds();
  strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeCurrent));
  printf("Data  Time: %s  Fiducial: 0x%05x Vector: %d\n", sTimeText, uFiducial, uVector);
  return 0;
}

} //namespace Pds 
