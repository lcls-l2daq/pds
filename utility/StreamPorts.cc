#include "StreamPorts.hh"

#include "pdsdata/xtc/L1AcceptEnv.hh"

using namespace Pds;

//
//  Ranges assume the following limits:
//    Max Partitions       = 16
//    Max Level2/partition = 32
//
static const int MaxSegGroups    = Pds::L1AcceptEnv::MaxReadoutGroups;
static const int MaxSegMasters   = 8;
static const int MaxPartitionL1s = 32;
static const int MaxPartitionL2s = 64;
//static const int MaxBLDServers   = 32;
static const int BaseMcastAddr = 0xefff1010;

// EVR -> L1 : 0xefff1010
static const int SegmentMcastAddr  = BaseMcastAddr;
// L1  -> L2 : 0xefff1210
static const int EventMcastAddr    = SegmentMcastAddr +StreamPorts::MaxPartitions * MaxSegGroups * MaxSegMasters;
// L2  -> L0 : 0xefff1410
static const int ControlMcastAddr  = EventMcastAddr   +StreamPorts::MaxPartitions * MaxPartitionL2s;
// EVR -> L1(soft triggers) 0xefff1418
static const int ObserverMcastAddr = ControlMcastAddr +StreamPorts::MaxPartitions;
// VMON server<->client     0xeffff1420
static const int VmonMcastAddr     = ObserverMcastAddr+StreamPorts::MaxPartitions;
// EVR Master -> Slaves     0xeffff1428
static const int EvrMcastAddr      = VmonMcastAddr    +StreamPorts::MaxPartitions;
// L1 -> nowhere : 0xeffff1430
static const int SinkMcastAddr     = EvrMcastAddr     +StreamPorts::MaxPartitions;
// L2 -> mon
static const int MonReqMcastAddr   = SinkMcastAddr    +1;

// BLD -> L1,L2 : 0xefff1800
static const int BLDMcastAddr      = 0xefff1800;  // FIXED value for external code

static const unsigned PortBase         = 10150;
static const unsigned SegmentPortBase  = PortBase;                                                      // 10150
static const unsigned EventPortBase    = SegmentPortBase +StreamPorts::MaxPartitions * MaxSegGroups * MaxSegMasters;    // 10662
static const unsigned ControlPortBase  = EventPortBase   +StreamPorts::MaxPartitions * MaxPartitionL1s; // 10918
static const unsigned ObserverPortBase = ControlPortBase +StreamPorts::MaxPartitions * MaxPartitionL2s; // 11430
static const unsigned VmonPortBase     = ObserverPortBase+StreamPorts::MaxPartitions;                   // 11438
static const unsigned EvrPortBase      = VmonPortBase    +1;                                            // 11439
static const unsigned SinkPortBase     = EvrPortBase     +StreamPorts::MaxPartitions;                   // 11447
static const unsigned MonReqPortBase   = SinkPortBase    +1;
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
    // dstid -> segment group id
    return Ins(SegmentMcastAddr + (srcid*MaxSegGroups + dstid) * StreamPorts::MaxPartitions + partition,
         SegmentPortBase + dstid*StreamPorts::MaxPartitions + partition);
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

Ins StreamPorts::sink()
{
  return Ins(SinkMcastAddr, SinkPortBase);
}

Ins StreamPorts::monRequest(unsigned partition)
{
  return Ins(MonReqMcastAddr + partition, MonReqPortBase + partition);
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
