#ifndef ODFSEGSTREAMS_HH
#define ODFSEGSTREAMS_HH

#include "pds/utility/WiredStreams.hh"

namespace Pds {

  class PartitionMember;
  class SegWireSettings;

  class SegStreams : public WiredStreams {
  public:
    SegStreams(PartitionMember&, 
               const SegWireSettings&,
               const char* name);

    virtual ~SegStreams();
  };

}
#endif
