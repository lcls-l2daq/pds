#ifndef Pds_Opal1kManager_hh
#define Pds_Opal1kManager_hh

#include "pds/camera/CameraManager.hh"
#include "pds/camera/FexFrameServer.hh"

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

  public:
    virtual void allocate      (Transition* tr);
    virtual void doConfigure   (Transition* tr);
    virtual void nextConfigure (Transition* tr);
    //    virtual void unconfigure   (Transition* tr);

    virtual InDatagram* recordConfigure  (InDatagram* in);
  private:
    void _configure(const void*);

  private:
    CfgCache*                  _fexConfig;
    FexFrameServer*            _server;
    GenericPool*               _occPool;
    unsigned                   _grabberId;
  };
};

#endif
