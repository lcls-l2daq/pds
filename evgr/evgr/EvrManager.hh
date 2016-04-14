#ifndef PDSEVRMANAGER_HH
#define PDSEVRMANAGER_HH

#include "EvgrOpcode.hh"
#include <time.h>

namespace Pds {

  class CfgClientNfs;
  class Fsm;
  class Appliance;
  class Evr;
  class DoneTimer;
  class Server;
  class EvrFifoServer;
  class InletWire;
  class SegmentLevel;
  class VmonEvr;

  template <class T> class EvgrBoardInfo;

  class EvrManager {
  public:
    EvrManager(EvgrBoardInfo<Evr>& erInfo, CfgClientNfs& cfg, bool bTurnOffBeamCodes, unsigned module);
    ~EvrManager();

    // SIGINT handler
    static void sigintHandler(int);
    static void randomize_nodes(bool);

    Appliance&  appliance();
    Server&     server();
    void        setWire(InletWire* pWire) {_pWire = pWire;}
  private:
    Evr&        _er;
    Fsm&        _fsm;
    EvrFifoServer* _server;
    bool        _bTurnOffBeamCodes;
    unsigned    _module;
    InletWire*  _pWire;
    VmonEvr*    _vmon;
  };
}

#endif
