#ifndef ODFEVENTSTREAMS_HH
#define ODFEVENTSTREAMS_HH

#include "pds/utility/WiredStreams.hh"

namespace Pds {

  class PartitionMember;

  class EventStreams: public WiredStreams {
  public:
    EventStreams(PartitionMember& cmgr);
    virtual ~EventStreams();
  public:
    virtual unsigned netbufdepth() const;
    virtual unsigned EbDepth    () const;
    virtual unsigned MaxSize    () const;
  };

}
#endif
