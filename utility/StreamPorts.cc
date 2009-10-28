#include "StreamPorts.hh"

using namespace Pds;

//
//  Ranges assume the following limits:
//    Max Partitions       = 16
//    Max Level2/partition = 64
//    Max Level3/partition = 16
//
static const int MaxPartitionL1s = 64;
static const int MaxPartitionL2s = 64;
static const int MaxBLDServers   = 32;
static const int BaseMcastAddr = 0xefff1100;

// EVR -> L1 : 0xefff1110
static const int SegmentMcastAddr  = BaseMcastAddr    +StreamPorts::MaxPartitions; 
// L1  -> L2 : 0xefff1120
static const int EventMcastAddr    = SegmentMcastAddr +StreamPorts::MaxPartitions;
// L2  -> L0 : 0xefff1620
static const int ControlMcastAddr  = EventMcastAddr   +StreamPorts::MaxPartitions*MaxPartitionL2s;
// VMON server<->client // 0xeffff1630
static const int VmonMcastAddr     = ControlMcastAddr +StreamPorts::MaxPartitions;
// BLD -> L1,L2 : 0xefff1800
static const int BLDMcastAddr      = 0xefff1800;  // FIXED value for external code

static const unsigned PortBase         = 10002;
static const unsigned SegmentPortBase  = PortBase;                           // 10002
static const unsigned EventPortBase    = SegmentPortBase+1;                  // 10003
static const unsigned ControlPortBase  = EventPortBase   +MaxPartitionL1s;   // 10067
static const unsigned VmonPortBase     = ControlPortBase +MaxPartitionL2s;   // 10131
static const unsigned BLDPortBase      = 10148;   // FIXED value for external code


Ins StreamPorts::bcast(unsigned    partition,
                       Level::Type level,
		       unsigned    srcid)
{
  switch(level) {
  case Level::Event:
    return Ins(EventMcastAddr    + partition*MaxPartitionL2s + MaxPartitionL2s-1, 
	       EventPortBase     + srcid);
  case Level::Control:
    return Ins(ControlMcastAddr  + partition, 
	       ControlPortBase   + srcid);
  default:
    break;
  }
  return Ins();
}

Ins StreamPorts::event(unsigned    partition,
                       Level::Type level,
                       unsigned    dstid,
                       unsigned    srcid)
{
  switch(level) {
  case Level::Segment:
    return Ins(SegmentMcastAddr + partition,
	       SegmentPortBase);
  case Level::Event:
    return Ins(EventMcastAddr + partition*MaxPartitionL2s + dstid,
	       EventPortBase + srcid);
  case Level::Control:
    return Ins(ControlMcastAddr + partition,
	       ControlPortBase + srcid);
  default:
    break;
  }
  return Ins();
}

Ins StreamPorts::bld(unsigned id)
{
  return Ins(BLDMcastAddr + id, BLDPortBase);
}

Ins StreamPorts::vmon(unsigned partition)
{
  return Ins(VmonMcastAddr + partition, VmonPortBase);
}

StreamPorts::StreamPorts()
{
  for (int stream=0; stream<StreamParams::NumberOfStreams; stream++) {
    _ports[stream] = 0;
  }
}

StreamPorts::StreamPorts(const StreamPorts& rhs) 
{
  for (int stream=0; stream<StreamParams::NumberOfStreams; stream++) {
    _ports[stream] = rhs._ports[stream];
  }
}

void StreamPorts::port(unsigned stream, unsigned short value) {
  _ports[stream] = value;
}

unsigned short StreamPorts::port(unsigned stream) const {
  return _ports[stream];
}
