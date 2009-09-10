#ifndef PDSACQMANAGER_HH
#define PDSACQMANAGER_HH

#include <string>

namespace Pds 
{

class Fsm;
class Appliance;
class CfgClientNfs;
class EpicsArchMonitor;

class EpicsArchManager 
{
public:
    EpicsArchManager(CfgClientNfs& cfg, const std::string& sFnConfig, float fMinTriggerInterval);
    ~EpicsArchManager();

    Appliance& appliance() { return *_pFsm; }
    int writeMonitoredContent( Datagram& dg );

    GenericPool* getPool() { return _pPool; }
    // event handlers
    int onActionMap();
    
private:  
    static const Src srcLevel; // Src for Epics Archiver

    int initMonitor();   

    Fsm*                _pFsm;
    Action*             _pActionConfig;
    Action*             _pActionMap;
    Action*             _pActionL1Accept;
    Action*             _pActionDisable;        
    
    std::string         _sFnConfig;
    float               _fMinTriggerInterval;
    EpicsArchMonitor*   _pMonitor;    
    GenericPool*        _pPool;     
};

} // namespace Pds

#endif
