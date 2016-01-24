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
    EpicsArchManager(CfgClientNfs & cfg, const std::string & sFnConfig,
         float fMinTriggerInterval, int iDebugLevel, int iIgnoreLevel);
    ~EpicsArchManager();

    Appliance & appliance()
    {
      return *_pFsm;
    }
    int validate();
    int update  ();
    int writeMonitoredContent(Datagram & dg, UserMessage ** msg, const struct timespec& tsCurrent, unsigned int uVectorCur);

    GenericPool *getPool()
    {
      return _pPool;
    }
    
    void setNumEventNode(int iNumEventNode);
    
    int ignoreLevel()   // if > 0, ignore PV connection errors (1=ignore with message, 2=ignore silently)
    {
      return _iIgnoreLevel;
    }

    // event handlers
    int onActionMap(UserMessage*& pMsg);
    int onActionReset();

  private:
    static const Src srcLevel;  // Src for Epics Archiver

    int initMonitor(std::string& sErrorMessage);

    const Src & _src;
    Fsm *_pFsm;
    Action *_pActionReset;
    Action *_pActionConfig;
    Action *_pActionMap;
    Action *_pActionUnmap;
    Action *_pActionL1Accept;
    Action *_pActionUpdate;

    std::string       _sFnConfig;
    float             _fMinTriggerInterval;
    int               _iDebugLevel;
    int               _iIgnoreLevel;
    int               _iNumEventNode;
    EpicsArchMonitor* _pMonitor;
    GenericPool *     _pPool;
    GenericPool *     _occPool;
  };

}       // namespace Pds

#endif
