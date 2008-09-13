#ifndef Pds_InDatagram_hh
#define Pds_InDatagram_hh

#include <sys/socket.h>

namespace Pds {

  class Datagram;
  class InDatagramIterator;
  class Pool;
  class ToEb;
  class Ins;

  class InDatagram {
  public:
    virtual ~InDatagram() {}

    virtual const Datagram& datagram() const = 0;

    virtual InDatagramIterator* iterator(Pool*) const = 0;

    virtual int send(ToEb&, const Ins&) = 0;

  };

}

#endif
