#ifndef Xpm_MonServer_hh
#define Xpm_MonServer_hh

namespace Pds {
  namespace Xpm {
    class MonGroup;
    class MonServer {
    public:
      MonServer(unsigned port);
    public:
      unsigned env();
    private:
      std::list<MonGroup*> _groups;
    };
  };
};

#endif
