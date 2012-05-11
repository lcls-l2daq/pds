#include "StreamPorts.hh"

using namespace Pds;

//
//  Ranges assume the following limits:
//    Max Partitions       = 16
//    Max Level2/partition = 32
//
static const int MaxPartitionL1s = 32;
static const int MaxPartitionL2s = 64;
static const int MaxBLDServers   = 32;
static const int BaseMcastAddr = 0xefff1100;

// EVR -> L1 : 0xefff1100
static const int SegmentMcastAddr  = BaseMcastAddr;
// L1  -> L2 : 0xefff1110
static const int EventMcastAddr    = SegmentMcastAddr +StreamPorts::MaxPartitions;
// L2  -> L0 : 0xefff1510
static const int ControlMcastAddr  = EventMcastAddr   +StreamPorts::MaxPartitions*MaxPartitionL2s;
// EVR -> L1(soft triggers) 0xefff1520
static const int ObserverMcastAddr = ControlMcastAddr +StreamPorts::MaxPartitions;
// VMON server<->client // 0xeffff1520
static const int VmonMcastAddr     = ObserverMcastAddr+StreamPorts::MaxPartitions;
// EVR Master -> Slaves :
static const int EvrMcastAddr      = VmonMcastAddr+StreamPorts::MaxPartitions;

// BLD -> L1,L2 : 0xefff1800
static const int BLDMcastAddr      = 0xefff1800;  // FIXED value for external code

static const unsigned PortBase         = 10150;
static const unsigned SegmentPortBase  = PortBase;                                                    // 10150
static const unsigned EventPortBase    = SegmentPortBase +StreamPorts::MaxPartitions;                 // 10166
static const unsigned ControlPortBase  = EventPortBase   +StreamPorts::MaxPartitions*MaxPartitionL1s; // 10678
static const unsigned ObserverPortBase = ControlPortBase +StreamPorts::MaxPartitions*MaxPartitionL2s; // 11702
static const unsigned VmonPortBase     = ObserverPortBase+StreamPorts::MaxPartitions;                 // 11718
static const unsigned EvrPortBase      = VmonPortBase    +1;                                          // 11719
static const unsigned BLDPortBase      = 10148;   // FIXED value for external code


Ins StreamPorts::bcast(unsigned    partition,
                       Level::Type level,
		       unsigned    srcid)
{
  switch(level) {
  case Level::Event:
    return Ins(EventMcastAddr    + partition*MaxPartitionL2s + MaxPartitionL2s-1, 
	       EventPortBase     + partition*MaxPartitionL1s + srcid);
  case Level::Control:
    return Ins(ControlMcastAddr  + partition, 
	       ControlPortBase   + partition*MaxPartitionL2s + srcid);
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
	       SegmentPortBase  + partition);
  case Level::Event:
    return Ins(EventMcastAddr + partition*MaxPartitionL2s + dstid,
	       EventPortBase  + partition*MaxPartitionL1s + srcid);
  case Level::Control:
    return Ins(ControlMcastAddr + partition,
	       ControlPortBase  + partition*MaxPartitionL2s + srcid);
  case Level::Observer:   // special service from the EVR (software triggers)
    return Ins(ObserverMcastAddr + partition,
	       ObserverPortBase  + partition);
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

Ins StreamPorts::evr(unsigned partition)
{
  return Ins(EvrMcastAddr + partition, EvrPortBase + partition);
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
