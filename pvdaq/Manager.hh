#ifndef Pds_PvDaq_Manager_hh
#define Pds_PvDaq_Manager_hh

namespace Pds {
  class Appliance;
  class Fsm;
  namespace PvDaq {
    class Server;
    class Manager {
    public:
      Manager (Server&);
      ~Manager();
    public:
      Appliance& appliance();
    private:
      Fsm& _fsm;
    };
  }
}

#endif
