#include "StreamPorts.hh"

static const int EventMcastAddr  = 0xefff1100;
static const int BLDMcastAddr    = 0xefff5100;
static const unsigned EventPortBase = 10002;
static const unsigned BLDPortBase   = 10066;

using namespace Pds;

//
//  Event data is vectored by "eventID"
//
unsigned PdsStreamPorts::eventMcastAddr(unsigned partition,
					unsigned eventId)
{
  return EventMcastAddr + ((partition<<6) | eventId);
}

unsigned short PdsStreamPorts::eventPort(unsigned fragmentId)
{
  return EventPortBase  + fragmentId;
}

//
//  BLD goes to all partitions and all nodes (not vectored)
//
unsigned PdsStreamPorts::mcastAddrBld  (unsigned fragmentId)
{
  return BLDMcastAddr + fragmentId;
}

unsigned short PdsStreamPorts::portBld  (unsigned stream,
					 unsigned fragmentId)
{
  return BLDPortBase  + fragmentId + stream*128;
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
