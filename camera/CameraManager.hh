#ifndef Pds_CameraManager_hh
#define Pds_CameraManager_hh

#include "pds/camera/FexFrameServer.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pds/config/CfgCache.hh"

namespace PdsLeutron {
  class PicPortCL;
};

namespace Pds {

  class Appliance;
  class Server;

  class CfgCache;
  class Fsm;
  class DmaSplice;
  class Transition;
  class InDatagram;

  class CameraManager {
  public:
    CameraManager(const Src&,
		  CfgCache*);
    virtual ~CameraManager();

    Appliance&      appliance();
    FexFrameServer& server();

    // Unix signal handler
    static void sigintHandler(int);
    
  public:
    Transition* allocate      (Transition* tr);
    Transition* fetchConfigure(Transition* tr);
    Transition* doConfigure   (Transition* tr);
    Transition* nextConfigure (Transition* tr);
    Transition* unconfigure   (Transition* tr);

    InDatagram* recordConfigure  (InDatagram* in);

  public:
    void handle();
  private:
    void register_ (int);
    void unregister();

  private:
    virtual void _configure   (const void* tc)=0;

    virtual Pds::Damage _handle() { return 0; }
    virtual void _register  () {}
    virtual void _unregister() {}
  private:
    virtual PdsLeutron::PicPortCL& camera() = 0;

  private:
    DmaSplice*      _splice;
    FexFrameServer* _server;
    Fsm*            _fsm;
    int             _sig;
    CfgCache*       _camConfig;
    CfgCache*       _fexConfig;
    bool            _configured;
  protected:
    unsigned        _nposts;
  };
};

#endif
