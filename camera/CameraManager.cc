#include "pds/camera/CameraManager.hh"

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Damage.hh"

#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"

#include "pds/xtc/InDatagram.hh"

#include "pds/camera/DmaSplice.hh"
#include "pds/camera/PicPortCL.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/camera/FexFrameServer.hh"
#include "pds/camera/FrameServerMsg.hh"

#include "pds/config/FrameFexConfigType.hh"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <new>

//#define SIMULATE_EVR

#ifdef SIMULATE_EVR
#include "pds/service/Ins.hh"
#include "pds/service/Client.hh"
#include "pds/xtc/EvrDatagram.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/collection/Route.hh"

static Pds::Client* _outlet = 0;
static Pds::Ins     _dst;
#endif



Pds::CameraManager* signalHandlerArgs[64];

static void cameraSignalHandler(int arg)
{
  Pds::CameraManager* mgr = signalHandlerArgs[arg];
  mgr->handle();
}

namespace Pds {

  class FexConfig : public CfgCache {
  public:
    FexConfig(const Src& src) :
      CfgCache(src,_frameFexConfigType,sizeof(FrameFexConfigType)) {}
  private:
    int _size(void* tc) const { return reinterpret_cast<FrameFexConfigType*>(tc)->size(); }
  };

  class CameraMapAction : public Action {
  public:
    CameraMapAction(CameraManager& mgr) : _mgr(mgr) {}
    Transition* fire(Transition* tr) {
#ifdef SIMULATE_EVR
      _dst = StreamPorts::event(reinterpret_cast<Allocate*>(tr)->allocation().partitionid(),
				Level::Segment);
#endif
      return _mgr.allocate(tr); 
    }
    InDatagram* fire(InDatagram* dg) { return dg; }
  private:
    CameraManager& _mgr;
  };

  class CameraConfigAction : public Action {
  public:
    CameraConfigAction(CameraManager& mgr) : _mgr(mgr) {}
    Transition* fire(Transition* tr) { 
      tr = _mgr.fetchConfigure(tr);
      tr = _mgr.doConfigure   (tr);
      return tr; }
    InDatagram* fire(InDatagram* dg) { return _mgr.recordConfigure(dg); }
  private:
    CameraManager& _mgr;
  };

  class CameraUnconfigAction : public Action {
  public:
    CameraUnconfigAction(CameraManager& mgr) : _mgr(mgr) {}
    Transition* fire(Transition* tr) { return _mgr.unconfigure(tr); }
  private:
    CameraManager& _mgr;
  };

  class CameraBeginCalibAction : public Action {
  public:
    CameraBeginCalibAction(CameraManager& mgr) : _mgr(mgr) {}
    Transition* fire(Transition* tr) { return _mgr.doConfigure    (tr); }
    InDatagram* fire(InDatagram* tr) { return _mgr.recordConfigure(tr); }
  private:
    CameraManager& _mgr;
  };

  class CameraEndCalibAction : public Action {
  public:
    CameraEndCalibAction(CameraManager& mgr) : _mgr(mgr) {}
    Transition* fire(Transition* tr) { return _mgr.nextConfigure(tr); }
    InDatagram* fire(InDatagram* tr) { return tr; }
  private:
    CameraManager& _mgr;
  };
};

using namespace Pds;


CameraManager::CameraManager(const Src& src,
			     CfgCache*  camConfig) :
  _splice  (0),
  _server  (new FexFrameServer(src,*_splice)),
  _fsm     (new Fsm),
  _camConfig    (camConfig),
  _fexConfig    (new FexConfig(src)),
  _configured (false)
//   _configBuffer (new char[MaxConfigSize+sizeof(FrameFexConfigType)]),
//   _configService(new CfgClientNfs(src)),
//   _fextc        (_frameFexConfigType, src),
//   _fexConfig    (0)
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
  signalHandlerArgs[SIGINT] = this;

  if (sigaction(SIGINT, &int_action, 0) > 0) {
    printf("Couldn't set up SIGINT handler\n");
  }

}

CameraManager::~CameraManager()
{
  delete   _fsm;
  delete   _server;
  if (_splice) {
    delete   _splice;
    _splice = 0;
  }
  delete   _fexConfig;
  delete   _camConfig;
}

Transition* CameraManager::allocate (Transition* tr)
{
  const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
  _camConfig->init(alloc.allocation());
  _fexConfig->init(alloc.allocation());
  return tr;
}

Transition* CameraManager::fetchConfigure(Transition* tr)
{
  //
  //  retrieve the configuration
  //
  if (_camConfig->fetch(tr) <= 0)
    printf("Config::configure failed to retrieve camera configuration\n");

  if (_fexConfig->fetch(tr) <= 0)
    printf("Config::configure failed to retrieve FrameFex configuration\n");
 
  return tr;
}

Transition* CameraManager::doConfigure(Transition* tr)
{
  _configure(_camConfig->current());

  int ret;
  int sig;
  if ((sig = camera().SetNotification(PdsLeutron::LvCamera::NOTIFYTYPE_SIGNAL)) < 0) 
    printf("Camera::SetNotification: %s.\n", strerror(-sig));
  
  else {
    register_(sig);

    if ((ret = camera().Init()) < 0)
      printf("Camera::Init: %s.\n", strerror(-ret));
    
    else {
       if( _splice ) {
          _splice->initialize( camera().frameBufferBaseAddress(),
                               camera().frameBufferEndAddress ()- 
                               camera().frameBufferBaseAddress());
       }
      
      //
      //  The feature extraction needs to know 
      //  something about the camera output:
      //     offset, defective pixels, ...
      //
      _server->setFexConfig(*reinterpret_cast<const FrameFexConfigType*>(_fexConfig->current()));

      if ((ret = camera().Start()) < 0)
	printf("Camera::Start: %s.\n", strerror(-ret));
      else {
        _configured = true;
        return tr;
      }
    }
  }

  _camConfig->damage().increase(Damage::UserDefined);
  _fexConfig->damage().increase(Damage::UserDefined);

  return tr;
}

Transition* CameraManager::unconfigure(Transition* tr)
{
  if (true == _configured) {
    _configured = false;
    camera().Stop();
    unregister();
  }
  return tr;
}

Transition* CameraManager::nextConfigure    (Transition* tr)
{
  _camConfig->next();
  _fexConfig->next();
  return tr;
}

InDatagram* CameraManager::recordConfigure  (InDatagram* in) 
{
  _camConfig->record(in);
  _fexConfig->record(in);
  return in;
}

FexFrameServer& CameraManager::server() { return *_server; }

Appliance& CameraManager::appliance() { return *_fsm; }

void CameraManager::handle()
{
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

  _server->post(msg);
  _nposts++;
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

