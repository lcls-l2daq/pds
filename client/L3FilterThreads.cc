#include "pds/client/L3FilterThreads.hh"
#include "pds/client/L3FilterDriver.hh"

static std::vector<Pds::Appliance*> apps(create_m*   c_user,
                                         unsigned    nthreads,
                                         const char* expname)
{
  if (!nthreads) nthreads=4;
  std::vector<Pds::Appliance*> apps(nthreads);
  for(unsigned i=0; i<nthreads; i++)
    apps[i] = new Pds::L3FilterDriver(c_user(),expname);
  return apps;
}

Pds::L3FilterThreads::L3FilterThreads(create_m*   c_user,
                                      unsigned    nthreads,
                                      const char* expname) :
  Pds::WorkThreads("L3F",apps(c_user,nthreads,expname))
{
}

Pds::L3FilterThreads::~L3FilterThreads()
{
}
