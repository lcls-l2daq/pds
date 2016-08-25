#ifndef TaggedStreams_hh
#define TaggedStreams_hh

#include "pds/utility/WiredStreams.hh"

namespace Pds {

  class PartitionMember;

  class TaggedStreams : public WiredStreams {
  public:
    TaggedStreams(PartitionMember&, 
                  unsigned max_event_size,
                  unsigned max_event_depth,
                  const char* name);

    virtual ~TaggedStreams();
  };

}
#endif
