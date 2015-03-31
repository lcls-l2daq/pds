#ifndef ANDOR_MANAGER_HH
#define ANDOR_MANAGER_HH

#include <string>
#include <stdexcept>

#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pds/config/AndorConfigType.hh"
#include "pds/client/Fsm.hh"
#include "AndorOccurrence.hh"

namespace Pds
{

class Fsm;
class Appliance;
class CfgClientNfs;
class Action;
class Response;
class GenericPool;
class AndorServer;

class AndorManager
{
public:
  AndorManager(CfgClientNfs& cfg, int iCamera, bool bDelayMode, bool bInitTest, std::string sConfigDb, int iSleepInt, int iDebugLevel);
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
