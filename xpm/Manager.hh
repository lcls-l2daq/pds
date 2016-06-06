#ifndef Pds_Xpm_Manager_hh
#define Pds_Xpm_Manager_hh

namespace Pds {
  class Allocation;
  class Appliance;
  class CfgClientNfs;
  class Fsm;
  namespace Xpm {
    class Server;
    class Module;
    class StatsTimer;
    class Manager {
    public:
      Manager (Module&, Server&, CfgClientNfs&);
      ~Manager();
    public:
      Appliance& appliance();
    private:
      Fsm& _fsm;
      StatsTimer* _stats;
      Allocation* _alloc;
    };
  }
}

#endif
