#ifndef Pds_CameraManager_hh
#define Pds_CameraManager_hh

#include "pdsdata/xtc/Xtc.hh"
#include "pds/config/FrameFexConfigType.hh"

namespace PdsLeutron {
  class PicPortCL;
};

namespace Pds {

  class Appliance;
  class Server;

  class CfgClientNfs;
  class Fsm;
  class DmaSplice;
  class FexFrameServer;
  class Transition;
  class InDatagram;

  class CameraManager {
  public:
    CameraManager(const Src&,
		  unsigned maxConfigSize);
    virtual ~CameraManager();

    Appliance&      appliance();
    FexFrameServer& server();
    
  public:
    Transition* allocate   (Transition* tr);
    Transition* configure  (Transition* tr);
    Transition* unconfigure(Transition* tr);
    Transition* disable    (Transition* tr);

    InDatagram* configure  (InDatagram* in);
    InDatagram* unconfigure(InDatagram* in);

  public:
    void handle();
  private:
    void register_ (int);
    void unregister();

  private:
    virtual void _configure (char*)=0;
    virtual void _configure (InDatagram*)=0;

    virtual Pds::Damage _handle() { return 0; }
    virtual void _register  () {}
    virtual void _unregister() {}
  private:
    virtual PdsLeutron::PicPortCL& camera() = 0;
    virtual const TypeId& camConfigType() = 0;

  private:
    DmaSplice*      _splice;
    FexFrameServer* _server;
    Fsm*            _fsm;
    int             _sig;
    char*           _configBuffer;
    CfgClientNfs*   _configService;
    Xtc             _fextc;
    FrameFexConfigType* _fexConfig;
  protected:
    unsigned        _nposts;
  };
};

#endif
