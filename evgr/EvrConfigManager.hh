#ifndef Pds_EvrConfigManager_hh
#define Pds_EvrConfigManager_hh

#include "pds/config/EvrConfigType.hh"
#include "pds/utility/Transition.hh"
#include "pdsdata/xtc/Xtc.hh"

#include <vector>

namespace Pds {
  class Appliance;
  class CfgClientNfs;
  class Evr;
  class GenericPool;
  class InDatagram;
  class UserMessage;

  class EvrConfigManager {
  public:
    EvrConfigManager(Evr & er, CfgClientNfs & cfg, Appliance& app, bool bTurnOffBeamCodes);
    ~EvrConfigManager();
  public:
    void initialize(const Allocation& alloc);
    void insert(InDatagram * dg);
    const EvrConfigType* configure();
    void fetch(Transition* tr);
    void advance();
    void enable();
    void disable();
    void handleCommandRequest(const std::vector<unsigned>& codes);
  private:
    int _validate_groups(UserMessage*);
  private:
    Evr&                 _er;
    CfgClientNfs&        _cfg;
    Xtc                  _cfgtc;
    char *               _configBuffer;
    const EvrConfigType* _cur_config;
    const char*          _end_config;
    GenericPool*         _occPool;
    Appliance&           _app;
    Allocation           _alloc;
    bool                 _bTurnOffBeamCodes;
  };
};

#endif
