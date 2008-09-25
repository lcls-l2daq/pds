#ifndef ODFSEGSTREAMS_HH
#define ODFSEGSTREAMS_HH

//#include "AckStreams.hh"
#include "pds/utility/WiredStreams.hh"

namespace Pds {

class CollectionManager;
// class VmonAppliance;

class SegStreams : public WiredStreams {
public:
  enum { ebdepth     = 64 };

  SegStreams(CollectionManager&,int);

  virtual ~SegStreams();

private:
  //  VmonAppliance* _vmom_appliance;
};

}
#endif
