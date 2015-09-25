#ifndef PDS_STREAMPORTS_HH
#define PDS_STREAMPORTS_HH

#include "pds/service/Ins.hh"
#include "pdsdata/xtc/Level.hh"
#include "StreamParams.hh"

//
//  We build events from two types of data:
//    (1) fragments that vector event data within its partition and
//    (2) fragments that flood event data to all partitions
//
namespace Pds {
  class StreamPorts {
  public:
    enum { MaxPartitions=8 };
  public:
    ///  non-L1A datagram service
    static Ins bcast(unsigned    partition,
		     Level::Type level,
		     unsigned    srcid=0);
    ///  L1A datagram service
    static Ins event(unsigned    partition,
                     Level::Type level,
                     unsigned    dstid=0,
                     unsigned    srcid=0);
    ///  Beamline data service
    static Ins bld  (unsigned    id);
    ///  Vmon service
    static Ins vmon (unsigned    partition);
    ///  EVR master-slave sync service
    static Ins evr  (unsigned    partition);
    ///  Nowhere (via the network stack)
    static Ins sink ();
    ///  Mon request service
    static Ins monRequest(unsigned partition);
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
