#ifndef ODFEVENTSTREAMS_HH
#define ODFEVENTSTREAMS_HH

#include "pds/utility/WiredStreams.hh"

namespace Pds {

  class PartitionMember;

  class EventStreams: public WiredStreams {
  public:

#ifdef BUILD_LARGE_STREAM_BUFFER
    enum { netbufdepth = 16 };
#else
    enum { netbufdepth = 8 };
#endif

    enum { EbDepth = 16 };
    enum { MaxSize = 20*1024*1024 };
  public:
    EventStreams(PartitionMember& cmgr, 
		 unsigned max_size      = MaxSize,
		 unsigned net_buf_depth = netbufdepth,
		 unsigned eb_depth      = EbDepth);
    virtual ~EventStreams();
  };

}
#endif
