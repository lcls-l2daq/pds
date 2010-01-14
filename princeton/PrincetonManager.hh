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
    PrincetonManager(CfgClientNfs& cfg, const std::string& sFnOutput, int iDebugLevel);
    ~PrincetonManager();

    Appliance&    appliance() { return *_pFsm; }
    
    // Camera control: Gateway functions for accessing PrincetonServer class
    int configCamera(Princeton::ConfigV1& config);
    int unconfigCamera();
    int captureStart(int iShotIdStart);
    int captureEnd(int iShotIdEnd, InDatagram* in, InDatagram*& out);
    
private:          
    int                 _iDebugLevel;
    
    Fsm*                _pFsm;
    Action*             _pActionConfig;
    Action*             _pActionUnconfig;
    Action*             _pActionMap;
    Action*             _pActionL1Accept;
    Action*             _pActionDisable;        
    
    PrincetonServer*    _pServer;
    GenericPool*        _pPool;    
    bool                _bStreamMode;    
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
