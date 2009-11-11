#ifndef ODFSEGSTREAMS_HH
#define ODFSEGSTREAMS_HH

#include "pds/utility/WiredStreams.hh"

namespace Pds {

  class PartitionMember;

  class SegStreams : public WiredStreams {
  public:
    enum { ebdepth     = 32 };

    SegStreams(PartitionMember&);

    virtual ~SegStreams();
  };

}
#endif
