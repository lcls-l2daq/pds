#include "pds/camera/Opal1kManager.hh"

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/camera/FrameFexConfigV1.hh"
#include "pdsdata/opal1k/ConfigV1.hh"

#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"

#include "pds/xtc/InDatagram.hh"

#include "pds/camera/DmaSplice.hh"
#include "pds/camera/Opal1kCamera.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/camera/FexFrameServer.hh"
#include "pds/camera/FrameServerMsg.hh"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <new>

struct signalHandler {
  PdsLeutron::Opal1kCamera*   camera;
  Pds::FexFrameServer* server;
} signalHandlerArgs[64];

static void cameraSignalHandler(int arg)
{
  PdsLeutron::Opal1kCamera* camera = signalHandlerArgs[arg].camera;
  Pds::FrameServerMsg* msg = 
    new Pds::FrameServerMsg(Pds::FrameServerMsg::NewFrame,
			    camera->GetFrameHandle(),
			    camera->CurrentCount,
			    0);
  signalHandlerArgs[arg].server->post(msg);
}

static void registerCameraSignalHandler(int arg, 
					PdsLeutron::Opal1kCamera*   camera,
					Pds::FexFrameServer* server)
{
  struct sigaction action;
  if (server) {
    sigemptyset(&action.sa_mask);
    action.sa_handler  = cameraSignalHandler;
    action.sa_flags    = SA_RESTART;
    signalHandlerArgs[arg].camera=camera;
    signalHandlerArgs[arg].server=server;
  }
  else {
    action.sa_handler = SIG_DFL;
  }
  sigaction(arg,&action,NULL);
}

namespace Pds {

  class Opal1kMapAction : public Action {
  public:
    Opal1kMapAction(Opal1kManager&   config) : _config(config) {}
    Transition* fire(Transition* tr) { return _config.allocate(tr); }
    InDatagram* fire(InDatagram* dg) { return dg; }
  private:
    Opal1kManager& _config;
  };

  class Opal1kConfigAction : public Action {
  public:
    Opal1kConfigAction(Opal1kManager&   config) : _config(config) {}
    Transition* fire(Transition* tr) { return _config.configure(tr); }
    InDatagram* fire(InDatagram* dg) { return _config.configure(dg); }
  private:
    Opal1kManager& _config;
  };

  class Opal1kUnconfigAction : public Action {
  public:
    Opal1kUnconfigAction(Opal1kManager& config) : _config(config) {}
    Transition* fire(Transition* tr) { return _config.unconfigure(tr); }
    InDatagram* fire(InDatagram* dg) { return _config.unconfigure(dg); }
  private:
    Opal1kManager&   _config;
  };

  class Opal1kBeginRunAction : public Action {
  public:
    Transition* fire(Transition* tr) { return tr; }
    InDatagram* fire(InDatagram* tr) { return tr; }
  };

  class Opal1kEndRunAction : public Action {
  public:
    Transition* fire(Transition* tr) { return tr; }
    InDatagram* fire(InDatagram* tr) { return tr; }
  };
};

using namespace Pds;


Opal1kManager::Opal1kManager(const Src& src) :
  _camera  (new PdsLeutron::Opal1kCamera),
  _splice  (new DmaSplice),
  _server  (new FexFrameServer(src,*_splice)),
  _fsm     (new Fsm),
  _sig     (-1),
  _configBuffer(new char[sizeof(Opal1k::ConfigV1)+ 
			 Opal1k::ConfigV1::LUT_Size*sizeof(uint16_t) +
			 1000*sizeof(Camera::FrameCoord) +
			 sizeof(Camera::FrameFexConfigV1)]),
  _configService(new CfgClientNfs(src)),
  _opaltc(TypeId::Id_Opal1kConfig, src),
  _fextc(TypeId::Id_FrameFexConfig, src)
{
  _fsm->callback(TransitionId::Map        , new Opal1kMapAction     (*this));
  _fsm->callback(TransitionId::Configure  , new Opal1kConfigAction  (*this));
  _fsm->callback(TransitionId::BeginRun   , new Opal1kBeginRunAction);
  _fsm->callback(TransitionId::EndRun     , new Opal1kEndRunAction);
  _fsm->callback(TransitionId::Unconfigure, new Opal1kUnconfigAction(*this));
}

Opal1kManager::~Opal1kManager()
{
  delete   _fsm;
  delete[] _configBuffer;
  delete   _configService;
  delete   _server;
  delete   _splice;
  delete   _camera;
}

Transition* Opal1kManager::allocate (Transition* tr)
{
  const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
  _configService->initialize(alloc.allocation());
  return tr;
}

Transition* Opal1kManager::configure(Transition* tr)
{
  _opaltc.damage.increase(Damage::UserDefined);
  _fextc .damage.increase(Damage::UserDefined);

  printf("Configuring ...\n");
  //
  //  retrieve the configuration
  //
  char* cfgBuff = _configBuffer;
  int len;
  if ((len=_configService->fetch(*tr,TypeId::Id_Opal1kConfig, cfgBuff)) <= 0) {
    printf("Config::configure failed to retrieve Opal1000 configuration\n");
    return tr;
  }
  const Opal1k::ConfigV1& opalConfig = *new(cfgBuff) Opal1k::ConfigV1;
  cfgBuff += len;
  
  if ((len=_configService->fetch(*tr,TypeId::Id_FrameFexConfig, cfgBuff)) <= 0) {
    printf("Config::configure failed to retrieve FrameFex configuration\n");
    return tr;
  }
  const Camera::FrameFexConfigV1& fexConfig = *new(cfgBuff) Camera::FrameFexConfigV1;

  _fextc.damage = 0;

  _camera->Config(opalConfig);
  
  int ret;
  if ((_sig = _camera->SetNotification(PdsLeutron::LvCamera::NOTIFYTYPE_SIGNAL)) < 0) 
    printf("Camera::SetNotification: %s.\n", strerror(-_sig));
  
  else {
    printf("Registering handler for signal %d ...\n", _sig);
    registerCameraSignalHandler(_sig,_camera,_server);
    
    printf("Initializing ...\n");
    if ((ret = _camera->Init()) < 0)
      printf("Camera::Init: %s.\n", strerror(-ret));
    
    else {
      printf("DmaSplice::initialize %p 0x%x\n", 
	     _camera->FrameBufferBaseAddress,
	     _camera->FrameBufferEndAddress - _camera->FrameBufferBaseAddress);
      _splice->initialize( _camera->FrameBufferBaseAddress,
			  _camera->FrameBufferEndAddress - _camera->FrameBufferBaseAddress);
      
      //
      //  The feature extraction needs to know 
      //  something about the camera output:
      //     offset, defective pixels, ...
      //
      _server->Config(fexConfig,
		      opalConfig.output_offset());

      printf("Starting ...\n");
      if ((ret = _camera->Start()) < 0)
	printf("Camera::Start: %s.\n", strerror(-ret));
      else
	_opaltc.damage = 0;
    }
  }

  printf("Done\n");
  return tr;
}

Transition* Opal1kManager::unconfigure(Transition* tr)
{
  _camera->Stop();
  registerCameraSignalHandler(_sig,NULL,NULL);
  return tr;
}

InDatagram* Opal1kManager::configure  (InDatagram* in) 
{
  _opaltc.extent = sizeof(Xtc);
  _fextc .extent = sizeof(Xtc);
  char* cfgBuff = _configBuffer;
  const Opal1k::ConfigV1* opalConfig(0);
  const Camera::FrameFexConfigV1* fexConfig(0);

  if (_opaltc.damage.value()) {
    in->datagram().xtc.damage.increase(_opaltc.damage.value()); 
  }
  else {
    opalConfig = new(cfgBuff) Opal1k::ConfigV1;
    _opaltc.extent += opalConfig->size();
    cfgBuff        += opalConfig->size();

    if (_fextc.damage.value()) {
      in->datagram().xtc.damage.increase(_fextc.damage.value()); 
    }
    else {
      fexConfig = new(cfgBuff) Camera::FrameFexConfigV1;
      _fextc.extent += fexConfig->size();
    }
  }
  in->insert(_opaltc, opalConfig);
  in->insert(_fextc , fexConfig);

  return in;
}

InDatagram* Opal1kManager::unconfigure(InDatagram* in) 
{
  return in; 
}

Server& Opal1kManager::server() { return *_server; }

Appliance& Opal1kManager::appliance() { return *_fsm; }
