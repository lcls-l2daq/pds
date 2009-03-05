#ifndef Pds_Opal1kManager_hh
#define Pds_Opal1kManager_hh

#include "pdsdata/xtc/Xtc.hh"

namespace PdsLeutron {
  class Opal1kCamera;
};

namespace Pds {

  class Appliance;
  class Server;

  class CfgClientNfs;
  class Fsm;
  class Src;
  class DmaSplice;
  class FexFrameServer;
  class Transition;
  class InDatagram;

  class Opal1kManager {
  public:
    Opal1kManager(const Src& src);
    ~Opal1kManager();

    Appliance&  appliance();
    Server&     server();

  public:
    Transition* allocate (Transition* tr);
    Transition* configure(Transition* tr);
    Transition* unconfigure(Transition* tr);

    InDatagram* configure  (InDatagram* in);
    InDatagram* unconfigure(InDatagram* in);

  private:
    PdsLeutron::Opal1kCamera*   _camera;
    DmaSplice*      _splice;
    FexFrameServer* _server;
    Fsm*            _fsm;
    int             _sig;
    char*           _configBuffer;
    CfgClientNfs*   _configService;
    Xtc             _opaltc;
    Xtc             _fextc;
  };
};

#endif
