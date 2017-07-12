#ifndef Xpm_JitterCleaner_hh
#define Xpm_JitterCleaner_hh

#include "pds/cphw/Reg.hh"
#include "pds/cphw/Reg64.hh"

namespace Pds {
  namespace Xpm {

    class JitterCleaner {
    public:
      JitterCleaner();
      void init();
      void dump();
      //    private:
    public:
      Cphw::Reg   _reg[22];
    };
  };
};

#endif

