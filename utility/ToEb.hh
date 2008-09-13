#ifndef PDS_TOEB
#define PDS_TOEB

#include "pds/service/Client.hh"

struct iovec;

namespace Pds {

class CDatagram;
class ZcpDatagram;

class ToEb {
public:
  // The interface is required for outlet wires which send multicast
  // and it doesn't harm to have it there also for the other outlet
  // wires
  ToEb(int interface, 
       unsigned payloadsize, 
       unsigned maxdatagrams); 
  virtual ~ToEb() {}

  int  send(const CDatagram*  , const Ins&);
  int  send(const ZcpDatagram*, const Ins&);
  //  int  send(struct iovec*, unsigned iovCount, const Ins&);

private:
  Client _client;     // UDP client
};
}
#endif
