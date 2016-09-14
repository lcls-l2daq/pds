#ifndef Pds_L3FilterModule_hh
#define Pds_L3FilterModule_hh

#include "pds/client/WorkThreads.hh"

#include "pdsdata/app/L3FilterModule.hh"

namespace Pds {
  class L3FilterThreads : public WorkThreads {
  public:
    L3FilterThreads(create_m*, unsigned nthreads=0, const char* expname=0);
    ~L3FilterThreads();
  };
};
    
#endif
