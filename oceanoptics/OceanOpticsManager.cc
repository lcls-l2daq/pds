#include "OceanOpticsManager.hh"

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

#include "pds/config/OceanOpticsConfigType.hh"
#include "pds/config/OceanOpticsDataType.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/client/Action.hh"
#include "pds/client/Response.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/utility/StreamPorts.hh"
#include "OceanOpticsServer.hh"

using std::string;

namespace Pds 
{

static int printDataTime(const InDatagram* in);

class OceanOpticsMapAction : public Action 
{
public:
  OceanOpticsMapAction(OceanOpticsManager& manager, CfgClientNfs& cfg, int iDebugLevel) : 
    _manager(manager), _cfg(cfg), _iMapDeviceFail(0), _iDebugLevel(iDebugLevel) {}
  
  virtual Transition* fire(Transition* tr)     
  {
    const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
    _cfg.initialize(alloc.allocation());  
    
    _iMapDeviceFail = _manager.mapDevice(alloc.allocation());
    return tr;                
  }
  
  virtual InDatagram* fire(InDatagram* in) 
  {
    if (_iDebugLevel >= 1) 
      printf( "\n\n===== Writing Map Data =========\n" );          
    if (_iDebugLevel>=2) printDataTime(in);
    
    if ( _iMapDeviceFail != 0 )
      in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);      
    return in;
  }
private:
  OceanOpticsManager& _manager;
  CfgClientNfs&     _cfg;
  int               _iMapDeviceFail;    
  int               _iDebugLevel;
};

class OceanOpticsConfigAction : public Action 
{
public:
  OceanOpticsConfigAction(OceanOpticsManager& manager, CfgClientNfs& cfg, int iDebugLevel) :
      _manager(manager), _cfg(cfg), _iDebugLevel(iDebugLevel),
      _cfgtc(_typeOceanOpticsConfig, cfg.src()), _configDevice(), _iConfigDeviceFail(0)
      //, _occPool(sizeof(UserMessage),2)
  {}
  
  virtual Transition* fire(Transition* tr) 
  {
    int iConfigSize = _cfg.fetch(*tr, _typeOceanOpticsConfig, &_configDevice, sizeof(_configDevice));
    
    if ( iConfigSize == 0 ) // no config data is found in the database.       
    {
      printf( "OceanOpticsConfigAction::fire(): No config data is loaded. Will use default values for configuring the device.\n" );
      _configDevice = OceanOpticsConfigType(
        0.001  // Exposure time
      );
    }
            
    if ( iConfigSize != 0 && iConfigSize != sizeof(_configDevice) )
    {
      printf( "OceanOpticsConfigAction::fire(): Config data has incorrect size (%d B). Should be %d B.\n",
        iConfigSize, sizeof(_configDevice) );
        
      _configDevice       = OceanOpticsConfigType();
      _iConfigDeviceFail  = 1;
    }
                
    _iConfigDeviceFail = _manager.configDevice(_configDevice);
    
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
      _cfgtc.alloc( sizeof(_configDevice) );
      bConfigAllocated = true;
    }
    
    in->insert(_cfgtc, &_configDevice);
    
    if ( _iConfigDeviceFail != 0 )
    {
      in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
      
      //UserMessage* msg = new(&_occPool) UserMessage();
      //msg->append("Configuration failed\n");
      //_manager.appliance().post(msg);
    }
              
    printf( 
      "OceanOptics Config data:\n"
      "  Exposure time %g s\n",
      _configDevice.exposureTime()
      );
    printf( "  Wavelength Calibration Coefficients:\n    ");
    for (int iOrder = 0; iOrder < 4; ++iOrder)
      printf("[%d] %lg ", iOrder, _configDevice.waveLenCalib(iOrder));
    printf( "\n  Stray Light Constant: %lg\n", _configDevice.strayLightConstant());
    printf( "  Nonlinear Correction Coefficients:\n    ");
    for (int iOrder = 0; iOrder < 8; ++iOrder)
      printf("[%d] %lg ", iOrder, _configDevice.nonlinCorrect(iOrder)); 
    printf("\n");
    
    if (_iDebugLevel>=1) printf( "\nOutput payload size = %d\n", in->datagram().xtc.sizeofPayload());
    
    return in;
  }

private:
  OceanOpticsManager&   _manager;    
  CfgClientNfs&         _cfg;
  const int             _iDebugLevel;
  Xtc                   _cfgtc;
  OceanOpticsConfigType _configDevice;    
  int                   _iConfigDeviceFail;    
  //GenericPool       _occPool;
  
  /*
   * private static consts
   */
  static const TypeId _typeOceanOpticsConfig;    
};

/*
 * Definition of private static consts
 */
const TypeId OceanOpticsConfigAction::_typeOceanOpticsConfig = TypeId(TypeId::Id_OceanOpticsConfig, OceanOpticsConfigType::Version);

class OceanOpticsUnconfigAction : public Action 
{
public:
  OceanOpticsUnconfigAction(OceanOpticsManager& manager, int iDebugLevel) : _manager(manager), _iDebugLevel(iDebugLevel), 
   _iUnConfigDeviceFail(0)
  {}
      
  virtual Transition* fire(Transition* in) 
  {
    _iUnConfigDeviceFail = _manager.unconfigDevice();
    return in;
  }
  
  virtual InDatagram* fire(InDatagram* in) 
  {
    if (_iDebugLevel >= 1) 
      printf( "\n\n===== Writing Unmap Data =========\n" );          
    if (_iDebugLevel>=2) printDataTime(in);
    
    if ( _iUnConfigDeviceFail != 0 )
      in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);      
    return in;
  }    
private:
  OceanOpticsManager& _manager;
  int               _iDebugLevel;
  int               _iUnConfigDeviceFail;
};

class OceanOpticsBeginRunAction : public Action 
{
public:
  OceanOpticsBeginRunAction(OceanOpticsManager& manager, int iDebugLevel) : _manager(manager), _iDebugLevel(iDebugLevel),
   _iBeginRunDeviceFail(0)
  {}
      
  virtual Transition* fire(Transition* in) 
  {
    _iBeginRunDeviceFail = _manager.beginRunDevice();
    return in;
  }

  virtual InDatagram* fire(InDatagram* in) 
  {
    if (_iDebugLevel >= 1) 
      printf( "\n\n===== Writing BeginRun Data =========\n" );          
    if (_iDebugLevel>=2) printDataTime(in);
    
    if ( _iBeginRunDeviceFail != 0 )
      in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);      
    return in;
  }    
private:
  OceanOpticsManager& _manager;
  int               _iDebugLevel;
  int               _iBeginRunDeviceFail;
};

class OceanOpticsEndRunAction : public Action 
{
public:
  OceanOpticsEndRunAction(OceanOpticsManager& manager, int iDebugLevel) : _manager(manager), _iDebugLevel(iDebugLevel),
   _iEndRunDeviceFail(0)
  {}
      
  virtual Transition* fire(Transition* in) 
  {
    _iEndRunDeviceFail = _manager.endRunDevice();
    return in;
  }
  
  virtual InDatagram* fire(InDatagram* in) 
  {
    if (_iDebugLevel >= 1) 
      printf( "\n\n===== Writing EndRun Data =========\n" );          
    if (_iDebugLevel>=2) printDataTime(in);
    
    if ( _iEndRunDeviceFail != 0 )
      in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
    return in;
  }        
private:
  OceanOpticsManager& _manager;
  int               _iDebugLevel;
  int               _iEndRunDeviceFail;
};

class OceanOpticsL1AcceptAction : public Action 
{
public:
  OceanOpticsL1AcceptAction(OceanOpticsManager& manager, CfgClientNfs& cfg, int iDebugLevel) :
      _manager(manager), _cfg(cfg), _iDebugLevel(iDebugLevel)
  {
  }
        
  virtual InDatagram* fire(InDatagram* in)     
  {               
    int iL1AcceptFail = _manager.validateL1Data(in);    
    if (iL1AcceptFail != 0)
      in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);      
    
    return in;
  }

private:        
  OceanOpticsManager&   _manager;
  CfgClientNfs&       _cfg;
  int                 _iDebugLevel;
};


class OceanOpticsEnableAction : public Action 
{
public:
  OceanOpticsEnableAction(OceanOpticsManager& manager, int iDebugLevel) : 
   _manager(manager), _iDebugLevel(iDebugLevel),
   _iEnableDeviceFail(0)
  {}
      
  virtual Transition* fire(Transition* in) 
  {
    _iEnableDeviceFail = _manager.enableDevice();
    return in;
  }
  
  virtual InDatagram* fire(InDatagram* in)     
  {
    if ( _iEnableDeviceFail != 0 )
      in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
      
    if (_iDebugLevel >= 1) 
      printf( "\n\n===== Writing Enable Data =========\n" );
    if (_iDebugLevel>=2) printDataTime(in);
      
    return in;
  }
  
private:
  OceanOpticsManager& _manager;
  int               _iDebugLevel;
  int               _iEnableDeviceFail;
};

class OceanOpticsDisableAction : public Action 
{
public:
  OceanOpticsDisableAction(OceanOpticsManager& manager, int iDebugLevel) : 
   _manager(manager), _iDebugLevel(iDebugLevel), 
   _iDisableDeviceFail(0)
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
      
    return in;
  }
  
private:
  OceanOpticsManager& _manager;
  int               _iDebugLevel;
  int               _iDisableDeviceFail;    
};

OceanOpticsManager::OceanOpticsManager(CfgClientNfs& cfg, OceanOpticsServer* pServer, int iDevice, int iDebugLevel) :
  _pServer(pServer), _iDevice(iDevice), _iDebugLevel(iDebugLevel)
{
  _pActionMap       = new OceanOpticsMapAction      (*this, cfg, _iDebugLevel);
  _pActionConfig    = new OceanOpticsConfigAction   (*this, cfg, _iDebugLevel);
  _pActionUnconfig  = new OceanOpticsUnconfigAction (*this, _iDebugLevel);  
  _pActionBeginRun  = new OceanOpticsBeginRunAction (*this, _iDebugLevel);
  _pActionEndRun    = new OceanOpticsEndRunAction   (*this, _iDebugLevel);  
  _pActionEnable    = new OceanOpticsEnableAction   (*this, _iDebugLevel);
  _pActionDisable   = new OceanOpticsDisableAction  (*this, _iDebugLevel);
  _pActionL1Accept  = new OceanOpticsL1AcceptAction (*this, cfg, _iDebugLevel);

  _pFsm = new Fsm();    
  _pFsm->callback(TransitionId::Map,          _pActionMap);
  _pFsm->callback(TransitionId::Configure,    _pActionConfig);
  _pFsm->callback(TransitionId::Unconfigure,  _pActionUnconfig);
  _pFsm->callback(TransitionId::BeginRun,     _pActionBeginRun);
  _pFsm->callback(TransitionId::EndRun,       _pActionEndRun);
  _pFsm->callback(TransitionId::Enable,       _pActionEnable);            
  _pFsm->callback(TransitionId::Disable,      _pActionDisable);            
  _pFsm->callback(TransitionId::L1Accept,     _pActionL1Accept);
}

OceanOpticsManager::~OceanOpticsManager()
{   
  delete _pFsm;  
  delete _pActionL1Accept;
  delete _pActionDisable;
  delete _pActionEnable;
  delete _pActionEndRun;
  delete _pActionBeginRun;
  delete _pActionUnconfig;    
  delete _pActionConfig;
  delete _pActionMap; 
}

int OceanOpticsManager::mapDevice(const Allocation& alloc)
{
  return 0;
}

int OceanOpticsManager::configDevice(OceanOpticsConfigType& config)
{
  if (_pServer->configure(config) != 0)
    return 1;
    
  _uFiducialPrev  = 0;
  _uVectorPrev    = 0;
  return 0;
}

int OceanOpticsManager::unconfigDevice()
{
  if (_pServer->unconfigure() != 0)
    return 1;
  return 0;
}

int OceanOpticsManager::beginRunDevice()
{
  return 0;
}

int OceanOpticsManager::endRunDevice()
{
  return 0;
}

int OceanOpticsManager::enableDevice()
{
  return 0;
}

int OceanOpticsManager::disableDevice()
{
  return 0;
}

int OceanOpticsManager::validateL1Data(InDatagram* in)
{ 
  unsigned int uFiducialCur  = in->datagram().seq.stamp().fiducials();    
  unsigned int uVectorCur    = in->seq.stamp().vector();      
  
  if ( _uFiducialPrev != 0 )
  {
    const int iFiducialWrapAroundDiffMin = 65536; 
    if ( (uFiducialCur <= _uFiducialPrev && _uFiducialPrev < uFiducialCur+iFiducialWrapAroundDiffMin)
    )
    {
      printf( "OceanOpticsManager::validateL1Data(): Fid 0x%x (vec %d) followed 0x%x (vec %d) \n", 
        uFiducialCur, uVectorCur, _uFiducialPrev, _uVectorPrev );
    }
  }
  _uFiducialPrev  = uFiducialCur;
  _uVectorPrev    = uVectorCur;
  
  return 0;
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
