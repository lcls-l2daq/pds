#ifndef PDSEVSMANAGER_HH
#define PDSEVSMANAGER_HH

#include "EvgrOpcode.hh"

namespace Pds {

  class Appliance;
  class CfgClientNfs;
  class Evr;
  class EvrFifoServer;
  class Fsm;
  class Server;
  template <class T> class EvgrBoardInfo;

  class EvsManager {
  public:  
    EvsManager(EvgrBoardInfo<Evr>& erInfo, CfgClientNfs& cfg);
    ~EvsManager();

    // SIGINT handler
    static void sigintHandler(int);
    static void randomize_nodes(bool);
    
    Appliance&  appliance();
    Server&     server();
  private:
    Evr&        _er;
    Fsm&        _fsm;
    EvrFifoServer* _server;
  };
}

#endif
