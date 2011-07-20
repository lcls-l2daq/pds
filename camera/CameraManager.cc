#include "pds/camera/CameraManager.hh"

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Damage.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"

#include "pds/xtc/InDatagram.hh"

#include "pds/camera/DmaSplice.hh"
#include "pds/camera/PicPortCL.hh"
#include "pds/camera/FrameServer.hh"
#include "pds/camera/FrameServerMsg.hh"

#include "pds/config/CfgCache.hh"
#include "pds/config/CfgClientNfs.hh"

#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <new>

//#define SIMULATE_EVR
#define DBUG

#ifdef SIMULATE_EVR
#include "pds/service/Ins.hh"
#include "pds/service/Client.hh"
#include "pds/xtc/EvrDatagram.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/collection/Route.hh"

static Pds::Client* _outlet = 0;
static Pds::Ins     _dst;
#endif



static Pds::CameraManager* signalHandlerArgs[] = 
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static void cameraSignalHandler(int arg)
{
  Pds::CameraManager* mgr = signalHandlerArgs[arg];
  if (mgr)
    mgr->handle();
}

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


CameraManager::CameraManager(const Src& src,
			     CfgCache*  camConfig) :
  //  _splice  (new DmaSplice),
  _splice  (0),
  _src     (src),
  _fsm     (new Fsm),
  _camConfig  (camConfig),
  _configured (false),
  _occPool     (new GenericPool(sizeof(UserMessage),1)),
  _hsignal     ("CamSignal")
{
#ifdef DBUG
  _tsignal.tv_sec = _tsignal.tv_nsec = 0;
#endif
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
  signalHandlerArgs[SIGINT] = this;

  if (sigaction(SIGINT, &int_action, 0) > 0) {
    printf("Couldn't set up SIGINT handler\n");
  }
}

CameraManager::~CameraManager()
{
  delete   _fsm;
  if (_splice) {
    delete   _splice;
    _splice = 0;
  }
  delete   _camConfig;
}

void CameraManager::allocate (Transition* tr)
{
  const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
  _camConfig->init(alloc.allocation());
}

void CameraManager::doConfigure(Transition* tr)
{
  //
  //  retrieve the configuration
  //
  if (_camConfig->fetch(tr) > 0) {

    _configure(_camConfig->current());

    int ret;
    int sig;
    if ((sig = camera().SetNotification(PdsLeutron::LvCamera::NOTIFYTYPE_SIGNAL)) < 0) 
      printf("Camera::SetNotification: %s.\n", strerror(-sig));
  
    else {
      register_(sig);

      if ((ret = camera().Init()) < 0) {
	printf("Camera::Init: %s.\n", strerror(-ret));
	UserMessage* msg = new(_occPool) UserMessage;
	msg->append(DetInfo::name(static_cast<const DetInfo&>(_src)));
	msg->append(":Failed to initialize.\nCheck camera power and connections");
	appliance().post(msg);
      }
      else {
	if (_splice)
	  _splice->initialize( camera().frameBufferBaseAddress(),
			       camera().frameBufferEndAddress ()- 
			       camera().frameBufferBaseAddress());
      
	if ((ret = camera().Start()) < 0)
	  printf("Camera::Start: %s.\n", strerror(-ret));
	else {
	  _configured = true;
	  return;
	}
      }
    }
  }
  else {
    printf("Config::configure failed to retrieve camera configuration\n");
    UserMessage* msg = new(_occPool) UserMessage;
    msg->append(DetInfo::name(static_cast<const DetInfo&>(_src)));
    msg->append(":Failed to open config file");
    appliance().post(msg);
  }

  _camConfig->damage().increase(Damage::UserDefined);
}

void CameraManager::unconfigure(Transition* tr)
{
  if (true == _configured) {
    _configured = false;
    camera().Stop();
    unregister();
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

void CameraManager::handle()
{
#ifdef DBUG
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    if (_tsignal.tv_sec)
      _hsignal.accumulate(ts,_tsignal);
    _tsignal=ts;
#endif

#ifdef SIMULATE_EVR
  // simulate an EVR for testing
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ClockTime ctime(ts.tv_sec,ts.tv_nsec);
    Sequence seq(Sequence::Event,TransitionId::L1Accept,ctime,TimeStamp(0,_nposts));
    EvrDatagram datagram(seq, _nposts);
    _outlet->send((char*)&datagram,0,0,_dst);
#endif

  Pds::FrameServerMsg* msg = 
    new Pds::FrameServerMsg(Pds::FrameServerMsg::NewFrame,
			    camera().GetFrameHandle(),
			    _nposts,
			    0);

  msg->damage.increase(_handle().value());

  server().post(msg);
  _nposts++;

#ifdef DBUG
  _hsignal.print(_nposts);
#endif
}

void CameraManager::register_(int sig)
{
#ifdef SIMULATE_EVR
  _outlet = new Client(sizeof(EvrDatagram),0,
		       Ins(Route::interface())),
#endif

  _register();

  _sig    = sig;
  _nposts = 0;

  printf("signal %d registered to cammgr %p\n", sig, this); 

  signalHandlerArgs[sig] = this;
  struct sigaction action;
  sigemptyset(&action.sa_mask);
  action.sa_handler  = cameraSignalHandler;
  action.sa_flags    = SA_RESTART;
  sigaction(sig,&action,NULL);
}

void CameraManager::unregister()
{
#ifdef SIMULATE_EVR
  if (_outlet) {
    delete _outlet;
    _outlet = 0;
  }
#endif

  if (_sig >= 0) {
    struct sigaction action;
    action.sa_handler = SIG_DFL;
    sigaction(_sig,&action,NULL);
    _sig = -1;
  }

  _unregister();
}

void CameraManager::sigintHandler(int)
{
  printf("SIGINT received\n");

  Pds::CameraManager* mgr = signalHandlerArgs[SIGINT];

  if (mgr) {
    if (true == mgr->_configured) {
      mgr->_configured = false;
      printf("Calling camera().Stop()... ");
      mgr->camera().Stop();
      printf("done.\n");
      // unregister();
    }
    else {
      printf("%s: not configured\n", __FUNCTION__);
    }

    if (mgr->_splice) {
      delete mgr->_splice;
      mgr->_splice = 0;
    }
  }
  else {
    printf("%s: signal handler arg %d is NULL\n", __FUNCTION__, SIGINT);
  }

  exit(0);
}

