#ifndef Pds_FccdManager_hh
#define Pds_FccdManager_hh

#include "pds/camera/CameraManager.hh"
#include "pds/camera/FccdFrameServer.hh"

namespace Pds {

  class Src;
  class GenericPool;
  class FccdConfig;

  class FccdManager : public CameraManager {
  public:
    FccdManager(const Src& src);
    ~FccdManager();

  public:
    FrameServer& server();

  public:
    virtual void allocate      (Transition* tr);
    virtual bool doConfigure   (Transition* tr);
    virtual void nextConfigure (Transition* tr);
    //    virtual void unconfigure   (Transition* tr);

    // virtual InDatagram* recordConfigure  (InDatagram* in);
  private:
    void _configure(const void*);

  private:
    CfgCache*                  _fccdConfig;
    FccdFrameServer*           _server;
  };
};

#endif
