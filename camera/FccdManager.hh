#ifndef Pds_FccdManager_hh
#define Pds_FccdManager_hh

#include "pds/camera/CameraManager.hh"
#include "pds/camera/FccdFrameServer.hh"

namespace PdsLeutron {
  class FccdCamera;
};

namespace Pds {

  class Src;
  class GenericPool;
  class FccdConfig;

  class FccdManager : public CameraManager {
  public:
    FccdManager(const Src& src,
      unsigned   grabberId=0);

    ~FccdManager();

  public:
    FrameServer& server();

  private: // out of order checking
    // Pds::Damage _handle    ();
    void        _register  ();
    void        _unregister();

  public:
    void attach_camera();
    void detach_camera();

  public:
    virtual void allocate      (Transition* tr);
    virtual void doConfigure   (Transition* tr);
    virtual void nextConfigure (Transition* tr);
    //    virtual void unconfigure   (Transition* tr);

    virtual InDatagram* recordConfigure  (InDatagram* in);
  private:
    void _configure(const void*);

    PdsLeutron::PicPortCL& camera();

  private:
    CfgCache*                  _fccdConfig;
    FccdFrameServer*           _server;
    PdsLeutron::FccdCamera*  _camera;
    //  buffer management
    // bool            _outOfOrder;
    GenericPool*    _occPool;
    unsigned        _grabberId;
  };
};

#endif
