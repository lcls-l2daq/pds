#ifndef PRINCETON_MANAGER_HH
#define PRINCETON_MANAGER_HH

#include <string>
#include <stdexcept>

#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/princeton/ConfigV1.hh"
#include "pds/client/Fsm.hh"

namespace Pds 
{

class Fsm;
class Appliance;
class CfgClientNfs;
class Action;
class GenericPool;
class PrincetonServer;

class PrincetonManager 
{
public:
  PrincetonManager(CfgClientNfs& cfg, bool bMakeUpEvent, const std::string& sFnOutput, int iDebugLevel);
  ~PrincetonManager();

  Appliance&    appliance() { return *_pFsm; }
  
  // Camera control: Gateway functions for accessing PrincetonServer class
  int   mapCamera();
  int   configCamera(Princeton::ConfigV1& config);
  int   unconfigCamera();
  int   beginRunCamera();
  int   endRunCamera();
  int   onEventShotIdStart(int iShotIdStart);
  int   onEventShotIdEnd(int iShotIdEnd, InDatagram* in, InDatagram*& out);
  int   onEventShotIdUpdate(int iShotIdStart, int iShotIdEnd, InDatagram* in, InDatagram*& out);
  int   getMakeUpData(InDatagram* in, InDatagram*& out);
  
private:          
  const bool          _bMakeUpEvent;
  const bool          _bStreamMode;    
  const int           _iDebugLevel;
  
  Fsm*                _pFsm;
  Action*             _pActionMap;
  Action*             _pActionConfig;
  Action*             _pActionUnconfig;
  Action*             _pActionBeginRun;
  Action*             _pActionEndRun;
  Action*             _pActionDisable;  
  Action*             _pActionL1Accept;

  
  PrincetonServer*    _pServer;
};

class PrincetonManagerException : public std::runtime_error
{
public:
  explicit PrincetonManagerException( const std::string& sDescription ) :
    std::runtime_error( sDescription )
  {}  
};

} // namespace Pds

#endif
