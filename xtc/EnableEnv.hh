#ifndef Pds_EnableEnv_hh
#define Pds_EnableEnv_hh

#include "pdsdata/xtc/Env.hh"

namespace Pds {
  class ClockTime;
  class EnableEnv : public Env {
  public:
    EnableEnv(unsigned);
    EnableEnv(const ClockTime&);
  public:
    bool     timer   () const;
    unsigned duration() const;
    unsigned events  () const;
  };
};

#endif
