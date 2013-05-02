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
    enum { MaxPartitions=16 };
  public:
    static Ins bcast(unsigned    partition,
		     Level::Type level,
		     unsigned    srcid=0);
    static Ins event(unsigned    partition,
                     Level::Type level,
                     unsigned    dstid=0,
                     unsigned    srcid=0);
    static Ins bld  (unsigned    id);
    static Ins vmon (unsigned    partition);
    static Ins evr  (unsigned    partition);
    static Ins sink ();
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
