#ifndef Pds_MonSrc_hh
#define Pds_MonSrc_hh

#include "pdsdata/xtc/Src.hh"

namespace Pds {
  class MonSrc : public Src {
  public:
    MonSrc(unsigned id);
    ~MonSrc();
  public:
    unsigned id() const;
  };
};

#endif
