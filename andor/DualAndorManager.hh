#ifndef DUAL_ANDOR_MANAGER_HH
#define DUAL_ANDOR_MANAGER_HH

#include <string>
#include <stdexcept>

#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pds/service/Semaphore.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"
#include "pds/config/Andor3dConfigType.hh"
#include "pds/client/Fsm.hh"
#include "DualAndorOccurrence.hh"
#include "cadef.h"

namespace Pds
{

class Fsm;
class Appliance;
class CfgClientNfs;
class Action;
class Response;
class GenericPool;
class DualAndorServer;
class DualAndorManager;

class DualAndorPollRoutine : public Routine
{
  public:
    void SetRunning(int state);
    void SetTemperature(int tempMaster, int tempSlave);
    DualAndorPollRoutine(DualAndorManager& manager, std::string sTempMasterPV, std::string sTempSlavePV);
    void routine(void);
  private:
    int               _state;
    int               _tempMaster;
    int               _tempSlave;
    DualAndorManager& _manager;
    chid              _chanMaster;
    chid              _chanSlave;
    std::string       _sTempMasterPV;
    std::string       _sTempSlavePV;
};

class DualAndorManager
{
public:
  DualAndorManager(CfgClientNfs& cfg, int iCamera, bool bDelayMode, bool bInitTest, std::string sConfigDb, int iSleepInt, int iDebugLevel, std::string sTempMasterPV, std::string sTempSlavePV);
  ~DualAndorManager();

  Appliance&    appliance() { return *_pFsm; }

  // Camera control: Gateway functions for accessing DualAndorServer class
  int   initServer();
  int   map(const Allocation& alloc);
  int   config(Andor3dConfigType& config, std::string& sConfigWarning);
  int   unconfig();
  int   beginRun();
  int   endRun();
  int   beginCalibCycle();
  int   endCalibCycle();
  int   enable();
  int   disable();
  int   l1Accept(bool& bWait);

  int   checkExposureEventCode(unsigned);
  int   startExposure();
  int   getData (InDatagram* in, InDatagram*& out);
  int   waitData(InDatagram* in, InDatagram*& out);
  bool  inBeamRateMode();
  int   getDataInBeamRateMode(InDatagram* in, InDatagram*& out);
  int   getTemperatureMaster(bool bForceUpdate);
  int   getTemperatureSlave (bool bForceUpdate);

private:
  const int             _iCamera;
  const bool            _bDelayMode;
  const bool            _bInitTest;
  const std::string     _sConfigDb;
  const int             _iSleepInt;
  const int             _iDebugLevel;
  const std::string     _sTempMasterPV;
  const std::string     _sTempSlavePV;

  Fsm*                  _pFsm;
  Action*               _pActionMap;
  Action*               _pActionConfig;
  Action*               _pActionUnconfig;
  Action*               _pActionBeginRun;
  Action*               _pActionEndRun;
  Action*               _pActionBeginCalibCycle;
  Action*               _pActionEndCalibCycle;
  Action*               _pActionEnable;
  Action*               _pActionDisable;
  Action*               _pActionL1Accept;
  Response*             _pResponse;

  DualAndorServer*      _pServer;
  unsigned int          _uNumShotsInCycle;
  DualAndorOccurrence*  _occSend;
  Task*                 _pTaskPoll;
public:
  DualAndorPollRoutine*          _pPoll;
  Semaphore*            _sem;
};

class DualAndorManagerException : public std::runtime_error
{
public:
  explicit DualAndorManagerException( const std::string& sDescription ) :
    std::runtime_error( sDescription )
  {}
};

} // namespace Pds

#endif
