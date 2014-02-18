#include "pds/camera/CameraManager.hh"

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Damage.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"

#include "pds/xtc/InDatagram.hh"

#include "pds/camera/CameraDriver.hh"

#include "pds/config/CfgCache.hh"
#include "pds/config/CfgClientNfs.hh"

#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"

#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <new>

namespace Pds {

  class CameraMapAction : public Action {
  public:
    CameraMapAction(CameraManager& mgr) : _mgr(mgr) {}
    Transition* fire(Transition* tr) {
#ifdef SIMULATE_EVR
      _dst = StreamPorts::event(reinterpret_cast<Allocate*>(tr)->allocation().partitionid(),
				Level::Segment);
#endif
      _mgr.allocate(tr); 
      return tr;
    }
    InDatagram* fire(InDatagram* dg) { return dg; }
  private:
    CameraManager& _mgr;
  };

  class CameraConfigAction : public Action {
  public:
    CameraConfigAction(CameraManager& mgr) : _mgr(mgr) {}
    Transition* fire(Transition* tr) { _mgr.doConfigure(tr); return tr; }
    InDatagram* fire(InDatagram* dg) { return _mgr.recordConfigure(dg); }
  private:
    CameraManager& _mgr;
  };

  class CameraUnconfigAction : public Action {
  public:
    CameraUnconfigAction(CameraManager& mgr) : _mgr(mgr) {}
    Transition* fire(Transition* tr) { _mgr.unconfigure(tr); return tr; }
  private:
    CameraManager& _mgr;
  };

  class CameraBeginCalibAction : public Action {
  public:
    CameraBeginCalibAction(CameraManager& mgr) : _mgr(mgr) {}
    Transition* fire(Transition* tr) { _mgr.doConfigure    (tr); return tr; }
    InDatagram* fire(InDatagram* tr) { _mgr.recordConfigure(tr); return tr; }
  private:
    CameraManager& _mgr;
  };

  class CameraEndCalibAction : public Action {
  public:
    CameraEndCalibAction(CameraManager& mgr) : _mgr(mgr) {}
    Transition* fire(Transition* tr) { _mgr.nextConfigure(tr); return tr; }
    InDatagram* fire(InDatagram* tr) { return tr; }
  private:
    CameraManager& _mgr;
  };
};

using namespace Pds;

static CameraManager* signalHandlerArg = 0;


CameraManager::CameraManager(const Src&    src,
			     CfgCache*     camConfig) :
  _src        (src),
  _fsm        (new Fsm),
  _camConfig  (camConfig),
  _driver     (0),
  _configured (false),
  _occPool    (new GenericPool(sizeof(UserMessage),1))
{
  _fsm->callback(TransitionId::Map            , new CameraMapAction       (*this));
  _fsm->callback(TransitionId::Configure      , new CameraConfigAction    (*this));
  _fsm->callback(TransitionId::Unconfigure    , new CameraUnconfigAction  (*this));
//   _fsm->callback(TransitionId::BeginCalibCycle, new CameraBeginCalibAction(*this));
//   _fsm->callback(TransitionId::EndCalibCycle  , new CameraEndCalibAction  (*this));

  // Unix signal support
  struct sigaction int_action;

  int_action.sa_handler = CameraManager::sigintHandler;
  sigemptyset(&int_action.sa_mask);
  int_action.sa_flags = 0;
  int_action.sa_flags |= SA_RESTART;
  signalHandlerArg = this;

  if (sigaction(SIGINT, &int_action, 0) > 0) {
    printf("Couldn't set up SIGINT handler\n");
  }
}

CameraManager::~CameraManager()
{
  delete   _fsm;
  delete   _camConfig;
}

void CameraManager::allocate (Transition* tr)
{
  const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
  _camConfig->init(alloc.allocation());
}

bool CameraManager::doConfigure(Transition* tr)
{
  UserMessage* msg = new(_occPool) UserMessage;
  msg->append(DetInfo::name(static_cast<const DetInfo&>(_src)));

  //
  //  retrieve the configuration
  //
  if (_camConfig->fetch(tr) > 0) {

    _configure(_camConfig->current());

    int ret;
    if ((ret = driver().initialize(msg)) < 0) {
    }
    else {
      if ((ret = driver().start_acquisition(server(),appliance(),msg)) < 0)
	;
      else {
        _configured = true;
	delete msg;
        return true;
      }
    }
  }
  else {
    printf("Config::configure failed to retrieve camera configuration\n");
    msg->append(":Failed to open config file");
  }

  appliance().post(msg);

  _camConfig->damage().increase(Damage::UserDefined);

  return false;
}

void CameraManager::unconfigure(Transition* tr)
{
  if (true == _configured) {
    _configured = false;
    driver().stop_acquisition();
  }
}

void CameraManager::nextConfigure    (Transition* tr)
{
  _camConfig->next();
}

InDatagram* CameraManager::recordConfigure  (InDatagram* in) 
{
  _camConfig->record(in);
  return in;
}

Appliance& CameraManager::appliance() { return *_fsm; }

void CameraManager::sigintHandler(int)
{
  printf("SIGINT received\n");

  Pds::CameraManager* mgr = signalHandlerArg;

  if (mgr) {
    if (true == mgr->_configured) {
      mgr->_configured = false;
      printf("Calling driver().Stop()... ");
      mgr->driver().stop_acquisition();
      printf("done.\n");
    }
    else {
      printf("%s: not configured\n", __FUNCTION__);
    }
  }
  else {
    printf("%s: signal handler arg %d is NULL\n", __FUNCTION__, SIGINT);
  }

  exit(0);
}

void CameraManager::attach(CameraDriver* d)
{
  _driver = d;
}

void CameraManager::detach()
{
  delete _driver; 
}

CameraDriver& CameraManager::driver() 
{
  return *_driver;
}
