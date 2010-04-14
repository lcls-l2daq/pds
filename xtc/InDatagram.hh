#ifndef Pds_InDatagram_hh
#define Pds_InDatagram_hh

#include "pds/xtc/Datagram.hh"

#include <sys/socket.h>

namespace Pds {

  class Xtc;
  class InDatagramIterator;
  class Pool;
  class ToNetEb;
  class ToEb;
  class Ins;
  class OobServer;
  class TrafficDst;

  class InDatagram : public Datagram {
  public:
    InDatagram(const InDatagram& dg);
    InDatagram(const Datagram& dg);
    virtual ~InDatagram() {}

    const Datagram& datagram() const { return *this; }
    Datagram& datagram() { return *this; }

    virtual bool insert(const Xtc& tc, const void* payload) = 0;

    virtual InDatagramIterator* iterator(Pool*) const = 0;

    virtual int  send   (ToNetEb&, const Ins&) = 0;
    virtual int  send   (ToEb&) = 0;

    virtual TrafficDst* traffic(const Ins&) = 0;
    //    virtual int  unblock(OobServer&, char*) = 0;
  };

}

inline Pds::InDatagram::InDatagram(const Pds::InDatagram& dg) :
  Pds::Datagram(dg)
{
}

inline Pds::InDatagram::InDatagram(const Pds::Datagram& dg) :
  Pds::Datagram(dg)
{
}

#endif
