#ifndef Pds_L1AcceptEnv_hh
#define Pds_L1AcceptEnv_hh

#include "pdsdata/xtc/Env.hh"

namespace Pds {
  class L1AcceptEnv : public Env {
  public:  
    L1AcceptEnv();
    L1AcceptEnv(unsigned n);
    L1AcceptEnv(Env& env);
  public:
    uint32_t clientGroupMask();
  };
};

#endif
