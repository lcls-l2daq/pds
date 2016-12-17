#ifndef Pds_Archon_Manager_hh
#define Pds_Archon_Manager_hh

namespace Pds {
  class Appliance;
  class CfgClientNfs;
  class Fsm;
  namespace Archon {
    class Driver;
    class Server;
    class Manager {
    public:
      Manager (Driver&, Server&, CfgClientNfs&);
      ~Manager();
    public:
      Appliance& appliance();
    private:
      Fsm& _fsm;
    };
  }
}

#endif
