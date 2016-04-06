#ifndef Pds_LeCroy_Manager_hh
#define Pds_LeCroy_Manager_hh

namespace Pds {
  class Appliance;
  class Fsm;
  namespace LeCroy {
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
