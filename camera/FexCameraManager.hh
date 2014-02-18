#ifndef Pds_FexCameraManager_hh
#define Pds_FexCameraManager_hh

#include "pds/camera/CameraManager.hh"

namespace Pds {

  class FexFrameServer;

  class FexCameraManager : public CameraManager {
  public:
    FexCameraManager(const Src&, CfgCache*);
    virtual ~FexCameraManager();

  public:
    virtual FrameServer& server();

  public:
    virtual void allocate      (Transition* tr);
    virtual bool doConfigure   (Transition* tr);
    virtual void nextConfigure (Transition* tr);

    virtual InDatagram* recordConfigure  (InDatagram* in);

  protected:
    FexFrameServer* _server;
  };
};

#endif
