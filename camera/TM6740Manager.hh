#ifndef Pds_TM6740Manager_hh
#define Pds_TM6740Manager_hh

#include "pds/camera/CameraManager.hh"
#include "pds/camera/FexFrameServer.hh"

namespace PdsLeutron {
  class TM6740Camera;
};

namespace Pds {

  class Src;
  class GenericPool;

  class TM6740Manager : public CameraManager {
  public:
    TM6740Manager(const Src& src);
    ~TM6740Manager();

  public:
    FrameServer& server();

  public:
    virtual void allocate      (Transition* tr);
    virtual void doConfigure   (Transition* tr);
    virtual void nextConfigure (Transition* tr);
    virtual void unconfigure   (Transition* tr);

    virtual InDatagram* recordConfigure  (InDatagram* in);
  public:
    void _configure(const void* tc);

  private:
    CfgCache*                  _fexConfig;
    FexFrameServer*            _server;
    unsigned                   _grabberId;
  };
};

#endif
