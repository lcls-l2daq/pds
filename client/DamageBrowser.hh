#ifndef Pds_DamageBrowser_hh
#define Pds_DamageBrowser_hh

#include "pdsdata/xtc/Xtc.hh"

#include <list>

namespace Pds {
  class InDatagram;
  class DamageBrowser {
  public:
    DamageBrowser(const InDatagram& dg);
    ~DamageBrowser();
  public:
    const std::list<Xtc>& damaged() const;
  private:
    std::list<Xtc> _damaged;
  };
};

#endif
