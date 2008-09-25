#ifndef ODFEVENTSTREAMS_HH
#define ODFEVENTSTREAMS_HH

#include "pds/utility/WiredStreams.hh"

namespace Pds {

class CollectionManager;
//class VmonAppliance;

class EventStreams: public WiredStreams {
public:
  enum { netbufdepth = 32 };
  enum { ebdepth     = 10 };

  EventStreams(CollectionManager& cmgr,int);
  virtual ~EventStreams();

private:
  //  VmonAppliance* _vmom_appliance;
};

}
#endif
