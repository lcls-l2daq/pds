#ifndef FLI_MANAGER_HH
#define FLI_MANAGER_HH

#include <string>
#include <stdexcept>

#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pds/config/FliConfigType.hh"
#include "pds/client/Fsm.hh"

namespace Pds 
{

class Fsm;
class Appliance;
class CfgClientNfs;
class Action;
class Response;
class GenericPool;
class FliServer;

class FliManager 
{
public:
  FliManager(CfgClientNfs& cfg, int iCamera, bool bDelayMode, bool bInitTest, std::string sConfigDb, int iSleepInt, int iDebugLevel);
  ~FliManager();

  Appliance&    appliance() { return *_pFsm; }
  
  // Camera control: Gateway functions for accessing FliServer class
  int   map(const Allocation& alloc);
  int   config(FliConfigType& config, std::string& sConfigWarning);
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
  
  FliServer*    _pServer;  
  unsigned int        _uNumShotsInCycle;
};

class FliManagerException : public std::runtime_error
{
public:
  explicit FliManagerException( const std::string& sDescription ) :
    std::runtime_error( sDescription )
  {}  
};

} // namespace Pds

#endif
