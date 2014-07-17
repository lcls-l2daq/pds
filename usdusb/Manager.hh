#ifndef Pds_UsdUsb_Manager_hh
#define Pds_UsdUsb_Manager_hh

namespace Pds {
  class Appliance;
  class CfgClientNfs;
  class Fsm;
  namespace UsdUsb {
    class Server;
    class Manager {
    public:
      Manager (unsigned dev, Server&, CfgClientNfs&);
      ~Manager();
    public:
      Appliance& appliance();
      void testTimeStep(bool t) { tsp = t; };
      bool tsp;
    private:
      Fsm& _fsm;
    };
  }
}

#endif
