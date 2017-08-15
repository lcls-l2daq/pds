#ifndef Xpm_PVStats_hh
#define Xpm_PVStats_hh

#include <string>
#include <vector>

#include "pds/xpm/Module.hh"

namespace Pds_Epics {
  class PVWriter;
};

namespace Pds {
  namespace Xpm {

    class PVStats {
    public:
      PVStats();
      ~PVStats();
    public:
      void allocate(const std::string& title);
      void begin(const L0Stats& s);
      void update(const L0Stats& ns, const L0Stats& os, double dt);
      void update(const CoreCounts& nc, const CoreCounts& oc, double dt);
    private:
      std::vector<Pds_Epics::PVWriter*> _pv;
      L0Stats _begin;
    };
  };
};

#endif
