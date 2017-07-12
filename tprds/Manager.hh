#ifndef Pds_TprDS_Manager_hh
#define Pds_TprDS_Manager_hh

namespace Pds {
  class Appliance;
  class CfgClientNfs;
  class Fsm;
  namespace TprDS {
    class TprReg;
    class Server;
    class Manager {
    public:
      Manager (TprReg&, Server&, CfgClientNfs&, bool lmonitor=false);
      ~Manager();
    public:
      Appliance& appliance();
    private:
      Fsm& _fsm;
    };
  }
}

#endif
