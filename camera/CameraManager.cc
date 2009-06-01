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

#include "pds/service/GenericPool.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/OccurrenceId.hh"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <new>


Pds::CameraManager* signalHandlerArgs[64];

static void cameraSignalHandler(int arg)
{
  Pds::CameraManager* mgr = signalHandlerArgs[arg];
  mgr->handle();
}

namespace Pds {

  class CameraMapAction : public Action {
  public:
    CameraMapAction(CameraManager& mgr) : _mgr(mgr) {}
    Transition* fire(Transition* tr) {
      return _mgr.allocate(tr); 
    }
    InDatagram* fire(InDatagram* dg) { return dg; }
  private:
    CameraManager& _mgr;
  };

  class CameraConfigAction : public Action {
  public:
    CameraConfigAction(CameraManager& mgr) : _mgr(mgr) {}
    Transition* fire(Transition* tr) { return _mgr.configure(tr); }
    InDatagram* fire(InDatagram* dg) { return _mgr.configure(dg); }
  private:
    CameraManager& _mgr;
  };

  class CameraUnconfigAction : public Action {
  public:
    CameraUnconfigAction(CameraManager& mgr) : _mgr(mgr) {}
    Transition* fire(Transition* tr) { return _mgr.unconfigure(tr); }
    InDatagram* fire(InDatagram* dg) { return _mgr.unconfigure(dg); }
  private:
    CameraManager& _mgr;
  };

  class CameraBeginRunAction : public Action {
  public:
    Transition* fire(Transition* tr) { return tr; }
    InDatagram* fire(InDatagram* tr) { return tr; }
  };

  class CameraEndRunAction : public Action {
  public:
    Transition* fire(Transition* tr) { return tr; }
    InDatagram* fire(InDatagram* tr) { return tr; }
  };

  class CameraDisableAction : public Action {
  public:
    CameraDisableAction(CameraManager& mgr) : _mgr(mgr) {}
    Transition* fire(Transition* tr) { return _mgr.disable(tr); }
    InDatagram* fire(InDatagram* tr) { return tr; }
  private:
    CameraManager& _mgr;
  };
};

using namespace Pds;


CameraManager::CameraManager(const Src& src,
			     unsigned MaxConfigSize) :
  _splice  (new DmaSplice),
  _server  (new FexFrameServer(src,*_splice)),
  _fsm     (new Fsm),
  _configBuffer(new char[MaxConfigSize+sizeof(FrameFexConfigType)]),
  _configService(new CfgClientNfs(src)),
  _fextc(_frameFexConfigType, src),
  _fexConfig(0)
{
  _fsm->callback(TransitionId::Map        , new CameraMapAction     (*this));
  _fsm->callback(TransitionId::Configure  , new CameraConfigAction  (*this));
  _fsm->callback(TransitionId::BeginRun   , new CameraBeginRunAction);
  _fsm->callback(TransitionId::Disable    , new CameraDisableAction (*this));
  _fsm->callback(TransitionId::EndRun     , new CameraEndRunAction);
  _fsm->callback(TransitionId::Unconfigure, new CameraUnconfigAction(*this));
}

CameraManager::~CameraManager()
{
  delete   _fsm;
  delete[] _configBuffer;
  delete   _configService;
  delete   _server;
  delete   _splice;
}

Transition* CameraManager::allocate (Transition* tr)
{
  const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
  _configService->initialize(alloc.allocation());
  return tr;
}

Transition* CameraManager::configure(Transition* tr)
{
  _fextc.damage = 0;
  _fextc.damage.increase(Damage::UserDefined);
  _fexConfig    = 0;

  //
  //  retrieve the configuration
  //
  char* cfgBuff = _configBuffer;
  int len;
  if ((len=_configService->fetch(*tr,camConfigType(), cfgBuff)) <= 0) {
    printf("Config::configure failed to retrieve camera configuration\n");
    return tr;
  }

  cfgBuff += len;
  
  if ((len=_configService->fetch(*tr,_frameFexConfigType, cfgBuff)) <= 0) {
    printf("Config::configure failed to retrieve FrameFex configuration\n");
    return tr;
  }

  _fexConfig = new(cfgBuff) FrameFexConfigType;

  _configure(_configBuffer);

  int ret;
  int sig;
  if ((sig = camera().SetNotification(PdsLeutron::LvCamera::NOTIFYTYPE_SIGNAL)) < 0) 
    printf("Camera::SetNotification: %s.\n", strerror(-sig));
  
  else {
    register_(sig);

    if ((ret = camera().Init()) < 0)
      printf("Camera::Init: %s.\n", strerror(-ret));
    
    else {
      _splice->initialize( camera().frameBufferBaseAddress(),
			   camera().frameBufferEndAddress ()- 
			   camera().frameBufferBaseAddress());
      
      //
      //  The feature extraction needs to know 
      //  something about the camera output:
      //     offset, defective pixels, ...
      //
      _server->setFexConfig(*_fexConfig);

      if ((ret = camera().Start()) < 0)
	printf("Camera::Start: %s.\n", strerror(-ret));
      else
	_fextc.damage = 0;
    }
  }

  return tr;
}

Transition* CameraManager::unconfigure(Transition* tr)
{
  camera().Stop();
  unregister();
  return tr;
}

Transition* CameraManager::disable(Transition* tr)
{
  return tr;
}

InDatagram* CameraManager::configure  (InDatagram* in) 
{
  _fextc.extent = sizeof(Xtc);

  if (_fextc.damage.value())
    in->datagram().xtc.damage.increase(_fextc.damage.value()); 
  else {
    _fextc.extent += _fexConfig->size();
    in->insert(_fextc, _fexConfig);
  }

  _configure(in);

  return in;
}

InDatagram* CameraManager::unconfigure(InDatagram* in) 
{
  return in; 
}

FexFrameServer& CameraManager::server() { return *_server; }

Appliance& CameraManager::appliance() { return *_fsm; }

void CameraManager::handle()
{
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
  if (_sig >= 0) {
    struct sigaction action;
    action.sa_handler = SIG_DFL;
    sigaction(_sig,&action,NULL);
    _sig = -1;
  }

  _unregister();
}

