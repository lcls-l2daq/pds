#ifndef PDS_TONETEB
#define PDS_TONETEB

#include "pds/service/Client.hh"
#include "pds/service/ZcpFragment.hh"

struct iovec;

namespace Pds {

class CDatagram;
class ZcpDatagram;

class ToNetEb {
public:
  // The interface is required for outlet wires which send multicast
  // and it doesn't harm to have it there also for the other outlet
  // wires
  ToNetEb(int interface, 
	  unsigned payloadsize, 
	  unsigned maxdatagrams); 
  virtual ~ToNetEb() {}

  int  send(const CDatagram*, const Ins&);
  int  send(ZcpDatagram*    , const Ins&);

private:
  Client      _client;     // UDP client
  ZcpFragment _fragment;
};
}
#endif
