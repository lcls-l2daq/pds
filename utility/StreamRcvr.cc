#include <stdio.h>

#include "StreamRcvr.hh"

using namespace Pds;

StreamRcvr::StreamRcvr(int address, int mcast) :
  _address(address),
  _mcast(mcast)
{}

StreamRcvr::StreamRcvr(int address, int mcast, const StreamPorts& ports) :
  _address(address),
  _mcast(mcast),
  _ports(ports)
{}

StreamRcvr::StreamRcvr(int address, const StreamPorts& ports) :
  _address(address),
  _mcast(0),
  _ports(ports)
{}

StreamRcvr::StreamRcvr(const StreamRcvr& rhs) 
{
  _address = rhs._address;
  _mcast = rhs._mcast;
  _ports = rhs._ports;
}

int StreamRcvr::address() const {return _address;}
int StreamRcvr::mcast() const {return _mcast;}
const StreamPorts& StreamRcvr::ports() const {return _ports;}
void StreamRcvr::port(int stream, unsigned short value) 
{
  _ports.port(stream, value);
}

void StreamRcvr::print() const
{
  printf("address 0x%x, mcast 0x%x", _address, _mcast);
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    printf(" %d", _ports.port(s));
  }
  printf("\n");
}
