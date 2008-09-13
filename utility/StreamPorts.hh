#ifndef PDS_STREAMPORTS_HH
#define PDS_STREAMPORTS_HH

#include "pds/service/Ins.hh"
#include "StreamParams.hh"

//
//  We build events from two types of data:
//    (1) fragments that vector event data within its partition and
//    (2) fragments that flood event data to all partitions
//
namespace PdsStreamPorts {
  unsigned eventMcastAddr(unsigned partition,
			  unsigned eventId);
  unsigned mcastAddrBld  (unsigned fragmentId);

  unsigned short eventPort(unsigned fragment);
  unsigned short portBld  (unsigned stream,
			   unsigned fragmentId);
}

namespace Pds {
  class StreamPorts {
  public:
    StreamPorts();
    StreamPorts(const StreamPorts&);

    void           port(unsigned stream, unsigned short);
    unsigned short port(unsigned stream) const;
  private:
    unsigned short _ports[StreamParams::NumberOfStreams];
  };
}

#endif
