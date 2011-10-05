#ifndef EPICS_ARCH_MANAGER_HH
#define EPICS_ARCH_MANAGER_HH

#include <string>

namespace Pds 
{

class Fsm;
class Allocation;
class Appliance;
class CfgClientNfs;
class EpicsArchMonitor;
class GenericPool;

class EpicsArchManager 
{
public:
    EpicsArchManager(CfgClientNfs& cfg, const std::string& sFnConfig, float fMinTriggerInterval, int iDebugLevel);
    ~EpicsArchManager();

    Appliance& appliance() { return *_pFsm; }
    int writeMonitoredContent( Datagram& dg, UserMessage** msg=0 );

    GenericPool* getPool() { return _pPool; }
    // event handlers
    int onActionMap();
    
private:  
    static const Src srcLevel; // Src for Epics Archiver

    int initMonitor();   

    const Src&          _src;
    Fsm*                _pFsm;
    Action*             _pActionConfig;
    Action*             _pActionMap;
    Action*             _pActionL1Accept;
    Action*             _pActionDisable;        
    
    std::string         _sFnConfig;
    float               _fMinTriggerInterval;
    int                 _iDebugLevel;
    EpicsArchMonitor*   _pMonitor;    
    GenericPool*        _pPool;     
    GenericPool*        _occPool;
};

} // namespace Pds

#endif
