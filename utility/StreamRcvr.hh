#ifndef PDS_STREAMRCVR_HH
#define PDS_STREAMRCVR_HH

#include "StreamPorts.hh"

namespace Pds {
class StreamRcvr {
public:
  StreamRcvr(int address=0, int mcast=0);
  StreamRcvr(int address, int mcast, const StreamPorts& ports);
  StreamRcvr(int address, const StreamPorts& ports);
  StreamRcvr(const StreamRcvr& rhs);

  int address() const;
  int mcast() const;
  const StreamPorts& ports() const;
  void port(int stream, unsigned short value);
  void print() const;

private:
  int _address;
  int _mcast;
  StreamPorts _ports;
};
}
#endif
