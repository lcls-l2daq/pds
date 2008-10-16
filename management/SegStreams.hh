#ifndef ODFSEGSTREAMS_HH
#define ODFSEGSTREAMS_HH

//#include "AckStreams.hh"
#include "pds/utility/WiredStreams.hh"

namespace Pds {

class CollectionManager;
// class VmonAppliance;

class SegStreams : public WiredStreams {
public:
  enum { ebdepth     = 16 };

  SegStreams(CollectionManager&);

  virtual ~SegStreams();

private:
  //  VmonAppliance* _vmom_appliance;
};

}
#endif
