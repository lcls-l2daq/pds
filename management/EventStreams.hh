#ifndef ODFEVENTSTREAMS_HH
#define ODFEVENTSTREAMS_HH

#include "pds/utility/WiredStreams.hh"

namespace Pds {

class CollectionManager;
class EventOccHandler;
//class VmonAppliance;

class EventStreams: public WiredStreams {
public:
  enum { netbufdepth = 32 };

  EventStreams(CollectionManager& cmgr);
  virtual ~EventStreams();

private:
  EventOccHandler* _occ_handler;
  //  VmonAppliance* _vmom_appliance;
};

}
#endif
