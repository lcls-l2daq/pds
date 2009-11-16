#ifndef ODFEVENTSTREAMS_HH
#define ODFEVENTSTREAMS_HH

#include "pds/utility/WiredStreams.hh"

namespace Pds {

  class PartitionMember;

  class EventStreams: public WiredStreams {
  public:
    enum { netbufdepth = 8 };
    enum { EbDepth = 16 };
    enum { MaxSize = 16*1024*1024 };
  public:
    EventStreams(PartitionMember& cmgr, 
		 unsigned max_size      = MaxSize,
		 unsigned net_buf_depth = netbufdepth,
		 unsigned eb_depth      = EbDepth);
    virtual ~EventStreams();
  };

}
#endif
