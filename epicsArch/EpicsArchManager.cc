#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <new>
#include <errno.h>
#include <math.h>

#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pds/service/GenericPool.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/config/CfgClientNfs.hh"
#include "EpicsArchMonitor.hh"
#include "EpicsArchManager.hh"

using namespace Pds;
using std::string;

class EpicsArchAllocAction:public Action
{
public:
  EpicsArchAllocAction(EpicsArchManager & manager, CfgClientNfs & cfg):_manager(manager), _cfg(cfg), _iMapError(0)
  {
  }

  virtual Transition *fire(Transition * tr)
  {
    const Allocation & alloc = ((const Allocate &) *tr).allocation();
    _cfg.initialize(alloc);
            
    int iNumEventNode = 0;
    for (unsigned i = 0; i < alloc.nnodes(); i++)
      if (alloc.node(i)->level() == Level::Event)
        iNumEventNode++;
    _manager.setNumEventNode(iNumEventNode);
    
    UserMessage* msg = NULL;
    _iMapError = _manager.onActionMap(msg);
    
    if (msg != NULL)
      _manager.appliance().post(msg);
    return tr;
  }
  
  virtual InDatagram* fire(InDatagram* in) 
  {
    if ( _iMapError != 0 )
      in->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
    return in;
  }
private:
  EpicsArchManager & _manager;
  CfgClientNfs & _cfg;
  int            _iMapError;
};

class EpicsArchConfigAction:public Action
{
public:
  EpicsArchConfigAction(EpicsArchManager & manager, const Src & src,
      CfgClientNfs & cfg, int iDebugLevel):
    //_cfgtc(_epicsArchConfigType,src),
  _manager(manager), _src(src), _cfg(cfg), _iDebugLevel(iDebugLevel)
  {
  }

  // this is the first "phase" of a transition where
  // all CPUs configure in parallel.
  virtual Transition *fire(Transition * tr)
  {
    //_cfg.fetch(*tr,_epicsArchConfigType, &_config);
    _manager.validate();
    return tr;
  }

  // this is the second "phase" of a transition where everybody
  // records the results of configure (which will typically be
  // archived in the xtc file).
  virtual InDatagram *fire(InDatagram * in)
  {
    if (_iDebugLevel >= 1)
      printf("\n\n===== Writing Configs =====\n");

    // insert assumes we have enough space in the input datagram
    //dg->insert(_cfgtc, &_config);
    //if (_nerror) dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);

    UserMessage*msg = NULL;
    InDatagram *out = new(_manager.getPool())Pds::CDatagram(in->datagram());
    int iFail       = _manager.writeMonitoredContent(out->datagram(), &msg, (struct timespec) {0,0}, 0);

    /*
     * Possible failure modes for _manager.writeMonitoredConfigContent()
     *   cf. EpicsArchMonitor::writeToXtc() return codes
     * 
     * Error Code     Reason
     * 
     * 2              Memory pool size is not enough for storing PV data
     * 3              All PV write failed. No PV value is outputted
     * 4              Some PV values have been outputted, but some has write error
     * 5              Some PV values have been outputted, but some has not been connected
     */
    if (iFail != 0)
    {
      // set damage bit          
      out->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
    }

    if (msg != NULL)
      _manager.appliance().post(msg);

    if (_iDebugLevel >= 1)
      printf("\nOutput payload size = %d\n", out->datagram().xtc.sizeofPayload());

    return out;
  }

private:
  //EpicsArchConfigType _config;
  //Xtc _cfgtc;
  EpicsArchManager & _manager;
  Src _src;
  CfgClientNfs & _cfg;
  int _iDebugLevel;
};

class EpicsArchL1AcceptAction:public Action
{
public:
  EpicsArchL1AcceptAction(EpicsArchManager & manager,
        float fMinTriggerInterval,
        int iDebugLevel):_manager(manager),
    _fMinTriggerInterval(fMinTriggerInterval),
    _iDebugLevel(iDebugLevel)
  {
    tsPrev.tv_sec = tsPrev.tv_nsec = 0;
  }

  ~EpicsArchL1AcceptAction()
  {
  }

  virtual InDatagram *fire(InDatagram * in)
  {
    // Check delta time
    timespec tsCurrent;
    int iStatus = clock_gettime(CLOCK_REALTIME, &tsCurrent);
    if (iStatus)
    {
      printf("EpicsArchL1AcceptAction::fire():clock_gettime() Failed: %s\n",
       strerror(iStatus));
    }
    
    unsigned int uVectorCur = in->seq.stamp().vector();  
    
    if (_iDebugLevel >= 1)
      printf("\n\n===== Writing L1 Data =====\n");
    InDatagram *out = new(_manager.getPool())Pds::CDatagram(in->datagram());
    int iFail = _manager.writeMonitoredContent(out->datagram(), NULL, tsCurrent, uVectorCur);

    /*
     * Possible failure modes for _manager.writeMonitoredConfigContent()
     *   cf. EpicsArchMonitor::writeToXtc() return codes
     * 
     * Error Code     Reason
     * 
     * 2              Memory pool size is not enough for storing PV data
     * 3              All PV write failed. No PV value is outputted
     * 4              Some PV values have been outputted, but some has write error
     * 5              Some PV values have been outputted, but some has not been connected
     */
    if (iFail != 0)
    {
      // set damage bit
      out->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
    }

    if (_iDebugLevel >= 1)
      printf("\nOutput payload size = %d\n",
       out->datagram().xtc.sizeofPayload());

    tsPrev = tsCurrent;
    return out;
  }

private:
  EpicsArchManager & _manager;
  float _fMinTriggerInterval;
  int _iDebugLevel;
  timespec tsPrev;
};

class EpicsArchDisableAction:public Action
{
public:
  EpicsArchDisableAction(EpicsArchManager & manager):_manager(manager)
  {
  }

  virtual Transition *fire(Transition * in)
  {
    return in;
  }
private:
  EpicsArchManager & _manager;
};

const Src EpicsArchManager::srcLevel = Src(Level::Reporter);

EpicsArchManager::EpicsArchManager(CfgClientNfs & cfg, const std::string & sFnConfig, float fMinTriggerInterval, int iDebugLevel):
  _src(cfg.src()), _sFnConfig(sFnConfig), 
  _fMinTriggerInterval(fMinTriggerInterval), _iDebugLevel(iDebugLevel), _iNumEventNode(0),
  _pMonitor(NULL) // _pMonitor need to be initialized in the task thread
{
  _pFsm = new Fsm();
  _pActionMap = new EpicsArchAllocAction(*this, cfg);
  _pActionConfig = new EpicsArchConfigAction(*this, EpicsArchManager::srcLevel, cfg, _iDebugLevel); // Level::Reporter for Epics Archiver
  _pActionL1Accept =
    new EpicsArchL1AcceptAction(*this, _fMinTriggerInterval, _iDebugLevel);
  _pActionDisable = new EpicsArchDisableAction(*this);

  _pFsm->callback(TransitionId::Map, _pActionMap);
  _pFsm->callback(TransitionId::Configure, _pActionConfig);
  _pFsm->callback(TransitionId::L1Accept, _pActionL1Accept);
  _pFsm->callback(TransitionId::Disable, _pActionDisable);

  _pPool    = new GenericPool(EpicsArchMonitor::iMaxXtcSize, 8);
  _occPool  = new GenericPool(sizeof(UserMessage), 1);
}

EpicsArchManager::~EpicsArchManager()
{
  delete _occPool;
  delete _pPool;
  delete _pMonitor;

  delete _pActionDisable;
  delete _pActionL1Accept;
  delete _pActionConfig;
  delete _pActionMap;

  delete _pFsm;
}

void EpicsArchManager::setNumEventNode(int iNumEventNode)
{
  _iNumEventNode = iNumEventNode;  
}

int EpicsArchManager::onActionMap(UserMessage*& pMsg)
{
  // initialize thread-specific data  
  string sConfigFileWarning;
  int iError = initMonitor(sConfigFileWarning);
  
  //if (!sConfigFileWarning.empty() && pMsg == NULL)
  if (false) // Do not show the PV title error message, as it is too verbose
  {
    /*
     * Note: The UserMessage will only contains the first PV that doesn't have formal description
     * 
     * For full list of problematic PVs, users need to check the epicsArch output (logfile).
     */
    pMsg = new(_occPool) UserMessage();
    
    UserMessage& msg = *pMsg;
    msg.append("EpicsArch: PV Title Error.\n");
    msg.append(sConfigFileWarning.c_str());    
  }

  return iError;
}

int EpicsArchManager::initMonitor(string& sConfigFileWarning)
{
  delete _pMonitor;

  try
  {
    _pMonitor =
      new EpicsArchMonitor(_src, _sFnConfig, _fMinTriggerInterval, _iNumEventNode,
        *_occPool, _iDebugLevel, sConfigFileWarning);            
  }
  catch(string & sError)
  {
    printf
      ("EpicsArchManager::initMonitor(): new EpicsArchMonitor( %s )failed: %s\n",
       _sFnConfig.c_str(), sError.c_str());
    _pMonitor = NULL;
    return 1;
  }

  return 0;
}

int EpicsArchManager::writeMonitoredContent(Datagram & dg, UserMessage ** msg, const struct timespec& tsCurrent, unsigned int uVectorCur)
{
  if (_pMonitor == NULL)
  {
    printf
      ("EpicsArchManager::writeMonitoredConfigContent(): Epics Monitor has not been init-ed\n");
    return 100;
  }

  return _pMonitor->writeToXtc(dg, msg, tsCurrent, uVectorCur);
}

int EpicsArchManager::validate()
{
  return _pMonitor->validate();
}
