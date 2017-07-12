#ifndef Pds_Xpm2_Manager_hh
#define Pds_Xpm2_Manager_hh

namespace Pds {
  class Appliance;
  class CfgClientNfs;
  namespace Xpm2 {
    class Server;
    class Module;
    class Manager {
    public:
      Manager (Module&, Server&, CfgClientNfs&);
      ~Manager();
    public:
      Appliance& appliance();
    private:
      Appliance*  _app;
    };
  }
}

#endif
