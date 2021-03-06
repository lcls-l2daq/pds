#ifndef Xpm_PVCtrls_hh
#define Xpm_PVCtrls_hh

#include "pds/epicstools/EpicsCA.hh"

#include <string>
#include <vector>

namespace Pds {

  class Semaphore;

  namespace Xpm {

    class Module;

    class PVCtrls
    {
    public:
      PVCtrls(Module&, Semaphore& sem);
      ~PVCtrls();
    public:
      void allocate(const std::string& title);
      void update();
      void dump() const;
    public:
      Module& module();
      Semaphore& sem();
    private:
      std::vector<Pds_Epics::EpicsCA*> _pv;
      Module&  _m;
      Semaphore& _sem;
    };
  };
};

#endif
