#ifndef ODFEVENTSTREAMS_HH
#define ODFEVENTSTREAMS_HH

#include "pds/utility/WiredStreams.hh"

namespace Pds {

class CollectionManager;
//class VmonAppliance;

class EventStreams: public WiredStreams {
public:
  enum { netbufdepth = 32 };
  enum { ebdepth     = 4 };         //  Need to keep these numbers "small" until
  enum { MaxSize     = 256*1024 };  //  we decide upon increasing pipe buffer number/size.
                                    //  I can merge fragments within the event builder to 
                                    //  reduce the number used.

  EventStreams(CollectionManager& cmgr);
  virtual ~EventStreams();

private:
  //  VmonAppliance* _vmom_appliance;
};

}
#endif
