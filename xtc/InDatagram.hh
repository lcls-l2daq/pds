#ifndef Pds_InDatagram_hh
#define Pds_InDatagram_hh

#include <sys/socket.h>

namespace Pds {

  class Datagram;
  class Xtc;
  class InDatagramIterator;
  class Pool;
  class ToNetEb;
  class ToEb;
  class Ins;
  class OobServer;

  class InDatagram {
  public:
    virtual ~InDatagram() {}

    virtual const Datagram& datagram() const = 0;
    virtual Datagram& datagram() = 0;

    virtual bool insert(const Xtc& tc, const void* payload) = 0;

    virtual InDatagramIterator* iterator(Pool*) const = 0;

    virtual int  send   (ToNetEb&, const Ins&) = 0;
    virtual int  send   (ToEb&) = 0;
    //    virtual int  unblock(OobServer&, char*) = 0;
  };

}

#endif
