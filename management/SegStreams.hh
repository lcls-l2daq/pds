#ifndef ODFSEGSTREAMS_HH
#define ODFSEGSTREAMS_HH

#include "pds/utility/WiredStreams.hh"

namespace Pds {

  class PartitionMember;

  class SegStreams : public WiredStreams {
  public:
    SegStreams(PartitionMember&, 
               unsigned max_event_size,
               unsigned max_event_depth);

    virtual ~SegStreams();
  };

}
#endif
