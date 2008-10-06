#include "StreamPorts.hh"

//
//  Ranges assume the following limits:
//    Max Partitions       = 16
//    Max Level2/partition = 64
//    Max Level3/partition = 16
//
static const int MaxPartitions = 16;
static const int MaxPartitionL1s = 64;
static const int MaxPartitionL2s = 64;
static const int MaxPartitionL3s = 16;
static const int BaseMcastAddr = 0xefff1100;

// EVR -> L1 : 0xefff1110
static const int SegmentMcastAddr  = BaseMcastAddr    +MaxPartitions; 
// L1  -> L2 : 0xefff1120
static const int EventMcastAddr    = SegmentMcastAddr +MaxPartitions;
// L2  -> L3 : 0xefff1520
static const int RecorderMcastAddr = EventMcastAddr   +MaxPartitions*MaxPartitionL2s;
// L3  -> L0 : 0xefff1100
static const int ControlMcastAddr  = RecorderMcastAddr+MaxPartitions*MaxPartitionL3s;
// BLD -> L1,L2 : 0xefff1620
static const int BLDMcastAddr      = ControlMcastAddr +MaxPartitions;

static const unsigned PortBase         = 10002;
static const unsigned SegmentPortBase  = PortBase;
static const unsigned EventPortBase    = SegmentPortBase+1;
static const unsigned RecorderPortBase = EventPortBase   +MaxPartitionL1s;
static const unsigned ControlPortBase  = RecorderPortBase+MaxPartitionL2s;
static const unsigned BLDPortBase      = ControlPortBase +MaxPartitionL3s;

using namespace Pds;

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
  case Level::Recorder:
    return Ins(RecorderMcastAddr + partition*MaxPartitionL3s + dstid,
	       RecorderPortBase + srcid);
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
