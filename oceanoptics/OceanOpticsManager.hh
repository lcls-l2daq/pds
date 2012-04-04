#ifndef OCEANOPTICS_MANAGER_HH
#define OCEANOPTICS_MANAGER_HH

#include <string>
#include <stdexcept>

#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pds/config/OceanOpticsConfigType.hh"
#include "pds/client/Fsm.hh"

namespace Pds 
{

class Fsm;
class Appliance;
class CfgClientNfs;
class Action;
class Response;
class GenericPool;
class OceanOpticsServer;

class OceanOpticsManager 
{
public:
  OceanOpticsManager(CfgClientNfs& cfg, OceanOpticsServer* pServer, int iDevice, int iDebugLevel);
  ~OceanOpticsManager();

  Appliance&    appliance() { return *_pFsm; }
  
  // Device control: Gateway functions for accessing OceanOpticsServer class
  int   mapDevice(const Allocation& alloc);
  int   configDevice(OceanOpticsConfigType& config);
  int   unconfigDevice();
  int   beginRunDevice();
  int   endRunDevice();
  int   enableDevice();
  int   disableDevice();
  int   validateL1Data(InDatagram* in);
  
private:          
  OceanOpticsServer*  _pServer;
  unsigned int        _uFiducialPrev;
  unsigned int        _uVectorPrev;

  const int           _iDevice;
  const int           _iDebugLevel;
  
  Fsm*                _pFsm;
  Action*             _pActionMap;
  Action*             _pActionConfig;
  Action*             _pActionUnconfig;
  Action*             _pActionBeginRun;
  Action*             _pActionEndRun;
  Action*             _pActionEnable;  
  Action*             _pActionDisable;  
  Action*             _pActionL1Accept;  
};

} // namespace Pds

#endif
