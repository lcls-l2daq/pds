#ifndef Pds_Opal1kManager_hh
#define Pds_Opal1kManager_hh

#include "pds/camera/CameraManager.hh"
#include "pds/camera/FexFrameServer.hh"

namespace PdsLeutron {
  class Opal1kCamera;
};

namespace Pds {

  class Src;
  class GenericPool;
  class Opal1kConfig;

  class Opal1kManager : public CameraManager {
  public:
    Opal1kManager(const Src& src);
    ~Opal1kManager();

  public:
    FrameServer& server();

  private: // out of order checking
    Pds::Damage _handle    ();
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
    CfgCache*                  _fexConfig;
    FexFrameServer*            _server;
    PdsLeutron::Opal1kCamera*  _camera;
    //  buffer management
    bool            _outOfOrder;
    GenericPool*    _occPool;
  };
};

#endif
