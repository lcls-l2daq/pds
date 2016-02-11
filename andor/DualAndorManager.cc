#include "DualAndorManager.hh"

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

#include "pds/config/Andor3dDataType.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/client/Action.hh"
#include "pds/client/Response.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/utility/StreamPorts.hh"
#include "DualAndorServer.hh"

using std::string;

namespace Pds
{
DualAndorPollRoutine::DualAndorPollRoutine(DualAndorManager& manager, string sTempMasterPV, string sTempSlavePV) :
  _state(1), _tempMaster(0), _tempSlave(0), _manager(manager), _chanMaster(NULL), _chanSlave(NULL),
  _sTempMasterPV(sTempMasterPV), _sTempSlavePV(sTempSlavePV) {};

void DualAndorPollRoutine::SetRunning(int state)
{
    _state = state;
}

void DualAndorPollRoutine::SetTemperature(int tempMaster, int tempSlave)
{
    _tempMaster = tempMaster;
    _tempSlave  = tempSlave;
}

void DualAndorPollRoutine::routine(void)
{
    int result;

    ca_context_create(ca_enable_preemptive_callback);
    result = ca_create_channel(_sTempMasterPV.c_str(), NULL, NULL, 50, &_chanMaster);
    if (result != ECA_NORMAL) {
        fprintf(stderr, "CA error %s while creating channel!\n", ca_message(result));
        return;
    }
    result = ca_pend_io(3.0);
    if (result != ECA_NORMAL) {
        fprintf(stderr, "CA error %s while waiting for channel connection!\n", ca_message(result));
        return;
    }
    result = ca_create_channel(_sTempSlavePV.c_str(), NULL, NULL, 50, &_chanSlave);
    if (result != ECA_NORMAL) {
        fprintf(stderr, "CA error %s while creating channel!\n", ca_message(result));
        return;
    }
    result = ca_pend_io(3.0); 
    if (result != ECA_NORMAL) {
        fprintf(stderr, "CA error %s while waiting for channel connection!\n", ca_message(result));
        return;
    }

    for (;;) {
        _manager._sem->take();
        if (_state) {
            _tempMaster = _manager.getTemperatureMaster(true);
            _tempSlave  = _manager.getTemperatureSlave (true);
        }
        _manager._sem->give();
        ca_put(DBR_LONG, _chanMaster, &_tempMaster);
        ca_put(DBR_LONG, _chanSlave,  &_tempSlave);
        ca_flush_io();
        usleep(1000000);
    }
}

static int printDataTime(const InDatagram* in);

class DualAndorMapAction : public Action
{
public:
    DualAndorMapAction(DualAndorManager& manager, CfgClientNfs& cfg, int iDebugLevel) :
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
    DualAndorManager& _manager;
    CfgClientNfs&     _cfg;
    int               _iMapCameraFail;
    int               _iDebugLevel;
};

class DualAndorConfigAction : public Action
{
public:
    DualAndorConfigAction(DualAndorManager& manager, CfgClientNfs& cfg, bool bDelayMode, int iDebugLevel) :
        _manager(manager), _cfg(cfg), _occPool(sizeof(UserMessage),2), _bDelayMode(bDelayMode), _iDebugLevel(iDebugLevel),
        _cfgtc(_typeAndor3dConfig, cfg.src()), _config(), _iConfigCameraFail(0)
    {}

    virtual Transition* fire(Transition* tr)
    {
      int iConfigSize = _cfg.fetch(*tr, _typeAndor3dConfig, &_config, sizeof(_config));

      if ( iConfigSize == 0 ) // no config data is found in the database.
      {
        printf( "AndorConfigAction::fire(): No config data is loaded. Will use default values for configuring the camera.\n" );
        _config = Andor3dConfigType(
          16, // Width
          16, // Height
          2,  // NumSensors
          0,  // OrgX
          0,  // OrgX
          1,  // BinX
          1,  // BinX
          0.001,  // Exposure time
          25.0f,  // Cooling temperature
          Andor3dConfigType::ENUM_FAN_FULL,  // fan Mode
          0,  // baseline clamp
          0,  // high capacity
          1,  // Gain index
          1,  // Readout speed index
          1,  // Redout event code
          12500,  // Exposure start delay
          1   // number delay shots
        );
      }

      if ( iConfigSize != 0 && iConfigSize != sizeof(_config) )
      {
        printf( "AndorConfigAction::fire(): Config data has incorrect size (%d B). Should be %d B.\n",
          iConfigSize, (int) sizeof(_config) );

        _config       = Andor3dConfigType();
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

        if (_iDebugLevel>=2) printf( "Andor Config data:\n"
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
    DualAndorManager&   _manager;
    CfgClientNfs&       _cfg;
    GenericPool         _occPool;
    bool                _bDelayMode;
    const int           _iDebugLevel;
    Xtc                 _cfgtc;
    Andor3dConfigType   _config;
    int                 _iConfigCameraFail;

    /*
     * private static consts
     */
    static const TypeId _typeAndor3dConfig;
};

/*
 * Definition of private static consts
 */
const TypeId DualAndorConfigAction::_typeAndor3dConfig = TypeId(TypeId::Id_Andor3dConfig, Andor3dConfigType::Version);

class DualAndorUnconfigAction : public Action
{
public:
    DualAndorUnconfigAction(DualAndorManager& manager, int iDebugLevel) : _manager(manager), _iDebugLevel(iDebugLevel),
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
    DualAndorManager& _manager;
    int               _iDebugLevel;
    int               _iUnConfigCameraFail;
};

class DualAndorBeginRunAction : public Action
{
public:
    DualAndorBeginRunAction(DualAndorManager& manager, int iDebugLevel) : _manager(manager), _iDebugLevel(iDebugLevel),
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
    DualAndorManager& _manager;
    int               _iDebugLevel;
    int               _iBeginRunCameraFail;
};

class DualAndorEndRunAction : public Action
{
public:
    DualAndorEndRunAction(DualAndorManager& manager, int iDebugLevel) : _manager(manager), _iDebugLevel(iDebugLevel),
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
    DualAndorManager& _manager;
    int               _iDebugLevel;
    int               _iEndRunCameraFail;
};

class DualAndorL1AcceptAction : public Action
{
public:
    DualAndorL1AcceptAction(DualAndorManager& manager, CfgClientNfs& cfg, bool bDelayMode, int iDebugLevel) :
        _manager(manager), _cfg(cfg), _bDelayMode(bDelayMode), _iDebugLevel(iDebugLevel)
        //, _poolFrameData(1024*1024*8 + 1024, 16) // pool for debug
    {
    }

    virtual InDatagram* fire(InDatagram* in)
    {
      int         iFail = 0;
      InDatagram* out   = in;
      int tempMaster;
      int tempSlave;

      if (_manager._pPoll) {
          tempMaster = _manager.getTemperatureMaster(false);
          tempSlave  = _manager.getTemperatureSlave (false);
          _manager._pPoll->SetTemperature(tempMaster, tempSlave);
      }

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
      if (_manager.inBeamRateMode())
      {
        iFail =  _manager.getDataInBeamRateMode( in, out );

        if ( iFail != 0 )
          // set damage bit
          out->datagram().xtc.damage.increase(Pds::Damage::UserDefined);

        return out;
      }

      bool bWait = false;
      _manager.l1Accept(bWait);

      if (_iDebugLevel >= 1 && bWait)
      {
        printf( "\n\n===== Writing L1Accept Data =========\n" );
        if (_iDebugLevel>=2) printDataTime(in);
        printf( "\n" );
      }

      if (bWait)
        iFail =  _manager.waitData( in, out );
      else
        iFail =  _manager.getData ( in, out );

      if ( iFail != 0 )
        // set damage bit
        out->datagram().xtc.damage.increase(Pds::Damage::UserDefined);

      /*
       * The folloing code is used for debugging variable L1 data size
       */
      //int iDataSize = 1024*1024*8 + sizeof(Andor3dDataType);
      //out =
      // new ( &_poolFrameData ) CDatagram( in->datagram() );
      //out->datagram().xtc.alloc( sizeof(Xtc) + iDataSize );
      //unsigned char* pXtcHeader = (unsigned char*) out + sizeof(CDatagram);
      //
      //TypeId typeAndorFrame(TypeId::Id_Andor3dFrame, Andor3dDataType::Version);
      //Xtc* pXtcFrame =
      // new ((char*)pXtcHeader) Xtc(typeAndor3dFrame, _cfg.src() );
      //pXtcFrame->alloc( iDataSize );

      //!!debug
      //uint32_t        uVector           = out->datagram().seq.stamp().vector();
      //printf("sending out L1 with vector %d\n", uVector);

      if (_iDebugLevel >= 1 && out != in)
      {
        if (!bWait)
        {
          printf( "\n\n===== Writing L1Accept Data =========\n" );
          if (_iDebugLevel>=2) printDataTime(in);
        }

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
          Andor3dDataType& frameData = *(Andor3dDataType*) xtcFrame.payload();
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
      }

      return out;
    }

private:
    DualAndorManager&   _manager;
    CfgClientNfs&       _cfg;
    bool                _bDelayMode;
    int                 _iDebugLevel;
    //GenericPool         _poolFrameData; // pool for debug
};

class DualAndorBeginCalibCycleAction : public Action
{
public:
    DualAndorBeginCalibCycleAction(DualAndorManager& manager, int iDebugLevel) :
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
    DualAndorManager& _manager;
    int               _iDebugLevel;
    int               _iBeginCalibCycleCameraFail;
};

class DualAndorEndCalibCycleAction : public Action
{
public:
    DualAndorEndCalibCycleAction(DualAndorManager& manager, int iDebugLevel) :
     _manager(manager), _iDebugLevel(iDebugLevel),
     _iEndCalibCycleCameraFail(0)
    {}

    virtual Transition* fire(Transition* in)
    {
      _iEndCalibCycleCameraFail = _manager.endCalibCycle();
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
    DualAndorManager& _manager;
    int               _iDebugLevel;
    int               _iEndCalibCycleCameraFail;
};

class DualAndorEnableAction : public Action
{
public:
    DualAndorEnableAction(DualAndorManager& manager, int iDebugLevel) :
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
    DualAndorManager& _manager;
    int               _iDebugLevel;
    int               _iEnableCameraFail;
};

class DualAndorDisableAction : public Action
{
public:
    DualAndorDisableAction(DualAndorManager& manager, int iDebugLevel) :
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

      if (!_manager.inBeamRateMode())
      {
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
            Andor3dDataType& frameData = *(Andor3dDataType*) xtcFrame.payload();
            printf( "Frame Id Start %d ReadoutTime %f\n", frameData.shotIdStart(),
                    frameData.readoutTime() );
          }
        }
      }

      _iDisableCameraFail = _manager.disable();
      if ( _iDisableCameraFail != 0 )
        in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);

      if (out != in)
      {
        UserMessage* msg = new(&_occPool) UserMessage();
        msg->append("Run stopped before data is sent out,\n");
        msg->append("so the data is attached to the Disable event.");
        _manager.appliance().post(msg);
      }

      return out;
    }

private:
    DualAndorManager& _manager;
    GenericPool       _occPool;
    int               _iDebugLevel;
    int               _iDisableCameraFail;
};

//
//  **weaver
//
class DualAndorResponse : public Response {
public:
  DualAndorResponse(DualAndorManager& mgr, int iDebugLevel) :
    _manager(mgr), _iDebugLevel(iDebugLevel)
  {
  }
public:
  Occurrence* fire(Occurrence* occ) {
    if (_iDebugLevel >= 1)
      printf( "\n\n===== Get Occurance =========\n" );
    if (_iDebugLevel>=2) printDataTime(NULL);

    const EvrCommand& cmd = *reinterpret_cast<const EvrCommand*>(occ);
    if (!_manager.inBeamRateMode() && _manager.checkExposureEventCode(cmd.code)) {
      if (_iDebugLevel >= 1)
        printf( "Get command event code: %u\n", cmd.code );

      _manager.startExposure();
    }
    return 0;
  }
private:
  DualAndorManager& _manager;
  int               _iDebugLevel;
};

DualAndorManager::DualAndorManager(CfgClientNfs& cfg, int iCamera, bool bDelayMode, bool bInitTest,
                                   string sConfigDb, int iSleepInt, int iDebugLevel,
                                   string sTempMasterPV, string sTempSlavePV) :
  _iCamera(iCamera), _bDelayMode(bDelayMode), _bInitTest(bInitTest),
  _sConfigDb(sConfigDb), _iSleepInt(iSleepInt),
  _iDebugLevel(iDebugLevel), _sTempMasterPV(sTempMasterPV), _sTempSlavePV(sTempSlavePV),
  _pServer(NULL), _uNumShotsInCycle(0), _pPoll(NULL)
{
  _sem                    = new Semaphore           (Semaphore::FULL);
  _pActionMap             = new DualAndorMapAction      (*this, cfg, _iDebugLevel);
  _pActionConfig          = new DualAndorConfigAction   (*this, cfg, _bDelayMode, _iDebugLevel);
  _pActionUnconfig        = new DualAndorUnconfigAction (*this, _iDebugLevel);
  _pActionBeginRun        = new DualAndorBeginRunAction (*this, _iDebugLevel);
  _pActionEndRun          = new DualAndorEndRunAction   (*this, _iDebugLevel);
  _pActionBeginCalibCycle = new DualAndorBeginCalibCycleAction
                                                        (*this, _iDebugLevel);
  _pActionEndCalibCycle   = new DualAndorEndCalibCycleAction
                                                        (*this, _iDebugLevel);
  _pActionEnable          = new DualAndorEnableAction   (*this, _iDebugLevel);
  _pActionDisable         = new DualAndorDisableAction  (*this, _iDebugLevel);
  _pActionL1Accept        = new DualAndorL1AcceptAction (*this, cfg, _bDelayMode, _iDebugLevel);
  _pResponse              = new DualAndorResponse       (*this, _iDebugLevel);

  try
  {
  _pServer = new DualAndorServer(_iCamera, _bDelayMode, _bInitTest, cfg.src(), _sConfigDb, _iSleepInt, _iDebugLevel);
  }
  catch ( DualAndorServerException& eServer )
  {
    throw DualAndorManagerException( "AndorManager::AndorManager(): Server Initialization Failed" );
  }

  _occSend = new DualAndorOccurrence(this);
  _pServer->setOccSend(_occSend);

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

DualAndorManager::~DualAndorManager()
{
  if ( _pTaskPoll != NULL )
    _pTaskPoll->destroy(); // task object will destroy the thread and release the object memory by itself

  delete _pFsm;

  delete _sem;
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

int DualAndorManager::initServer()
{
  return _pServer->initSetup();
}

int DualAndorManager::map(const Allocation& alloc)
{
  int result = _pServer->map();
  if (_pTaskPoll == 0 && !_sTempMasterPV.empty() && !_sTempSlavePV.empty()) {
      _pPoll = new DualAndorPollRoutine(*this, _sTempMasterPV, _sTempSlavePV);
      _pTaskPoll = new Task(TaskObject("DualAndorTempPoll"));
      _pTaskPoll->call( _pPoll );
  }
  return result;
}

int DualAndorManager::config(Andor3dConfigType& config, std::string& sConfigWarning)
{
  int result;
  _sem->take();
  if ( !_bDelayMode )
    Pds::Andor3dConfig::setNumDelayShots(config,0);

  result = _pServer->config(config, sConfigWarning);
  _sem->give();
  return result;
}

int DualAndorManager::unconfig()
{
  int result;
  _sem->take();
  result = _pServer->unconfig();
  _sem->give();
  return result;
}

int DualAndorManager::beginRun()
{
  int result;
  _sem->take();
  result = _pServer->beginRun();
  _sem->give();
  return result;
}

int DualAndorManager::endRun()
{
  int result;
  _sem->take();
  result = _pServer->endRun();
  _sem->give();
  return result;
}

int DualAndorManager::beginCalibCycle()
{
  int result;
  _sem->take();
  _uNumShotsInCycle = 0;
  result = _pServer->beginCalibCycle();
  _sem->give();
  return result;
}

int DualAndorManager::endCalibCycle()
{
  int result;
  _sem->take();
  result = _pServer->endCalibCycle();
  _sem->give();
  return result;
}

int DualAndorManager::enable()
{
  int result;
  _sem->take();
  if (_pPoll)
      _pPoll->SetRunning(0);
  result = _pServer->enable();
  _sem->give();
  return result;

}

int DualAndorManager::disable()
{
  int result;
  _sem->take();
  result = _pServer->disable();
  if (_pPoll)
      _pPoll->SetRunning(1);
  _sem->give();
  return result;
}

int DualAndorManager::l1Accept(bool& bWait)
{
  if (!_pServer->isCapturingData())
  {
    bWait = false;
    return 0;
  }

  ++_uNumShotsInCycle;

  if (inBeamRateMode())
    bWait = true;
  else if (!_bDelayMode)
    bWait = false;
  else
    bWait = (_uNumShotsInCycle>= _pServer->config().numDelayShots());
  return 0;
}

int DualAndorManager::startExposure()
{
  return _pServer->startExposure();
}

/* !! recover from delay mode
int DualAndorManager::startExposurePrompt(int iShotId, InDatagram* in, InDatagram*& out)
{
  return _pServer->startExposurePrompt(iShotId, in, out);
}

int DualAndorManager::startExposureDelay(int iShotId, InDatagram* in)
{
  return _pServer->startExposureDelay(iShotId, in);
}
*/

int DualAndorManager::getData(InDatagram* in, InDatagram*& out)
{
  return _pServer->getData(in, out);
}

int DualAndorManager::waitData(InDatagram* in, InDatagram*& out)
{
  int iFail = _pServer->waitData(in, out);

  if (out != in)
    _uNumShotsInCycle = 0;

  return iFail;
}

int DualAndorManager::checkExposureEventCode(unsigned code)
{
  return (code == _pServer->config().exposureEventCode());
}

bool DualAndorManager::inBeamRateMode()
{
  return _pServer->inBeamRateMode();
}

int DualAndorManager::getDataInBeamRateMode(InDatagram* in, InDatagram*& out)
{
  return _pServer->getDataInBeamRateMode(in, out);
}

int DualAndorManager::getTemperatureMaster(bool bForceUpdate)
{
  return _pServer->getTemperatureMaster(bForceUpdate);
}

int DualAndorManager::getTemperatureSlave(bool bForceUpdate)
{
  return _pServer->getTemperatureSlave(bForceUpdate);
}

/*
 * Print the local timestamp and the data timestamp
 *
 * If the input parameter in == NULL, no data timestamp will be printed
 */
/*
 * Print the local timestamp and the data timestamp
 *
 * If the input parameter in == NULL, no data timestamp will be printed
 */
static int printDataTime(const InDatagram* in)
{
  static const char sTimeFormat[40] = "%02d_%02H:%02M:%02S"; /* Time format string */
  char sTimeText[40];

  timespec timeCurrent;
  clock_gettime( CLOCK_REALTIME, &timeCurrent );
  time_t timeSeconds = timeCurrent.tv_sec;
  strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeSeconds));

  printf("Local Time: %s.%09ld\n", sTimeText, timeCurrent.tv_nsec);

  if ( in == NULL ) return 0;

  const ClockTime clockCurDatagram  = in->datagram().seq.clock();
  uint32_t        uFiducial         = in->datagram().seq.stamp().fiducials();
  uint32_t        uVector           = in->datagram().seq.stamp().vector();
  timeSeconds                       = clockCurDatagram.seconds();
  strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeSeconds));
  printf("Data  Time: %s.%09u  Fiducial: 0x%05x Vector: %d\n", sTimeText, clockCurDatagram.nanoseconds(), uFiducial, uVector);
  return 0;
}

} //namespace Pds
