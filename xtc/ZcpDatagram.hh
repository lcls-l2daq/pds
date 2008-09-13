#ifndef Pds_ZcpDatagram_hh
#define Pds_ZcpDatagram_hh

#include "InDatagram.hh"
#include "Datagram.hh"
#include "pds/service/ZcpFragment.hh"
#include "pds/xtc/xtc.hh"

namespace Pds {

  class ZcpDatagram : public InDatagram {
  public:
    ZcpDatagram(const Datagram&);
    ZcpDatagram(const Datagram&, ZcpFragment* fragment);
    ZcpDatagram(const TC&, const Src&);
    ~ZcpDatagram();

    void* operator new(size_t, Pool*);
    void  operator delete(void* buffer);

    const Datagram& datagram() const;

    InDatagramIterator* iterator(Pool*) const;

    int send(ToEb&, const Ins&);
  public:
    LinkedList<ZcpFragment>& fragments();
    const LinkedList<ZcpFragment>& fragments() const;
  private:
    Datagram  _datagram;
    LinkedList<ZcpFragment> _fragments;
  };

}

inline Pds::ZcpDatagram::ZcpDatagram(const Datagram& dg) :
  _datagram(dg)
{
}

inline Pds::ZcpDatagram::ZcpDatagram(const Datagram& dg,
				     ZcpFragment* fragment) :
  _datagram(dg)
{
  _fragments.connect(fragment->reverse());
}

inline Pds::ZcpDatagram::ZcpDatagram(const TC& tc, const Src& src) :
  _datagram(tc,src)
{
}

inline void* Pds::ZcpDatagram::operator new(size_t size, Pds::Pool* pool)
  {
  return pool->alloc(size);
  }

inline void Pds::ZcpDatagram::operator delete(void* buffer)
  {
  Pds::RingPool::free(buffer);
  }

inline Pds::LinkedList<Pds::ZcpFragment>& Pds::ZcpDatagram::fragments()
{
  return _fragments;
}

inline const Pds::LinkedList<Pds::ZcpFragment>& Pds::ZcpDatagram::fragments() const
{
  return _fragments;
}

#endif
