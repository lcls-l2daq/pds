#ifndef ANDOR_MANAGER_HH
#define ANDOR_MANAGER_HH

#include <string>
#include <stdexcept>

#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pds/service/Semaphore.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"
#include "pds/config/AndorConfigType.hh"
#include "pds/client/Fsm.hh"
#include "AndorOccurrence.hh"
#include "cadef.h"

namespace Pds
{

class Fsm;
class Appliance;
class CfgClientNfs;
class Action;
class Response;
class GenericPool;
class AndorServer;
class AndorManager;

class PollRoutine : public Routine
{
  public:
    void SetRunning(int state);
    void SetTemperature(int temp);
    PollRoutine(AndorManager& manager, std::string _sTempPV);
    void routine(void);
  private:
    int           _state;
    int           _temp;
    AndorManager& _manager;
    chid          _chan;
    std::string   _sTempPV;
};

class AndorManager
{
public:
  AndorManager(CfgClientNfs& cfg, int iCamera, bool bDelayMode, bool bInitTest, std::string sConfigDb, int iSleepInt, int iDebugLevel, std::string sTempPV);
  ~AndorManager();

  Appliance&    appliance() { return *_pFsm; }

  // Camera control: Gateway functions for accessing AndorServer class
  int   initServer();
  int   map(const Allocation& alloc);
  int   config(AndorConfigType& config, std::string& sConfigWarning);
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

private:
  const int           _iCamera;
  const bool          _bDelayMode;
  const bool          _bInitTest;
  const std::string   _sConfigDb;
  const int           _iSleepInt;
  const int           _iDebugLevel;
  const std::string   _sTempPV;

  Fsm*                _pFsm;
  Action*             _pActionMap;
  Action*             _pActionConfig;
  Action*             _pActionUnconfig;
  Action*             _pActionBeginRun;
  Action*             _pActionEndRun;
  Action*             _pActionBeginCalibCycle;
  Action*             _pActionEndCalibCycle;
  Action*             _pActionEnable;
  Action*             _pActionDisable;
  Action*             _pActionL1Accept;
  Response*           _pResponse;

  AndorServer*        _pServer;
  unsigned int        _uNumShotsInCycle;
  AndorOccurrence*    _occSend;
  Task*               _pTaskPoll;
public:
  PollRoutine*        _pPoll;
  Semaphore*          _sem;
};

class AndorManagerException : public std::runtime_error
{
public:
  explicit AndorManagerException( const std::string& sDescription ) :
    std::runtime_error( sDescription )
  {}
};

} // namespace Pds

#endif
