#include "pds/client/L3FilterThreads.hh"
#include "pds/client/L3FilterDriver.hh"
#include "pds/client/WorkThreads.hh"

Pds::L3FilterThreads::L3FilterThreads(create_m* c_user,
                                      unsigned  nthreads,
                                      bool      lveto)
{
  if (!nthreads) nthreads=4;
  std::vector<Pds::Appliance*> apps(nthreads);
  for(unsigned i=0; i<nthreads; i++)
    apps[i] = new Pds::L3FilterDriver(c_user(),lveto);

  _work = new Pds::WorkThreads("L3F",apps);
}

Pds::L3FilterThreads::~L3FilterThreads()
{
}

Pds::Transition* Pds::L3FilterThreads::transitions(Pds::Transition* tr)
{
  return _work->transitions(tr);
}

Pds::InDatagram* Pds::L3FilterThreads::events(Pds::InDatagram* in)
{
  return _work->events(in);
}
