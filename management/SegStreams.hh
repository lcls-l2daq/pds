#ifndef ODFSEGSTREAMS_HH
#define ODFSEGSTREAMS_HH

//#include "AckStreams.hh"
#include "pds/utility/WiredStreams.hh"

namespace Pds {

  class PartitionMember;
  // class VmonAppliance;

  class SegStreams : public WiredStreams {
  public:
    enum { ebdepth     = 32 };

    SegStreams(PartitionMember&);

    virtual ~SegStreams();

  private:
    //  VmonAppliance* _vmom_appliance;
  };

}
#endif
