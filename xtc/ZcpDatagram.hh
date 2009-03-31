#ifndef Pds_ZcpDatagram_hh
#define Pds_ZcpDatagram_hh

#include "InDatagram.hh"
#include "Datagram.hh"
#include "pds/service/RingPool.hh"
#include "pds/service/Pool.hh"
#include "pds/service/ZcpFragment.hh"
#include "pds/service/ZcpStream.hh"

namespace Pds {

  class ToEb;
  class ToNetEb;

  class ZcpDatagram : public InDatagram {
  public:
    ZcpDatagram(const Datagram&);
    ZcpDatagram(const Datagram&, ZcpFragment& fragment);
    ZcpDatagram(const TypeId&, const Src&);
    ~ZcpDatagram();

    void* operator new(size_t, Pool*);
    void  operator delete(void* buffer);

    const Datagram& datagram() const;
    Datagram& datagram();

    bool insert(const Xtc& tc, const void* payload);

    InDatagramIterator* iterator(Pool*) const;

    int send(ToEb&);
    int send(ToNetEb&, const Ins&);
    int unblock(OobServer&, char*);

    int dump() const;
  public:
    //    int _insert(char*       ,int);
    int _insert(ZcpFragment&,int);
    int _insert(ZcpStream&  ,int,ZcpFragment&);
  private:
    ZcpDatagram(const ZcpDatagram& dg) : _datagram(dg._datagram) {}
    // Friends who can break up the datagram during transmission/iteration
    friend class ToEb;
    friend class ToNetEb;
    friend class ZcpDatagramIterator;

  private:
    Datagram    _datagram; // the header
    ZcpStream   _stream;   // the payload
  };

}

inline Pds::ZcpDatagram::ZcpDatagram(const Datagram& dg) :
  _datagram(dg)
{
}

inline Pds::ZcpDatagram::ZcpDatagram(const Datagram& dg,
				     ZcpFragment& fragment) :
  _datagram(dg)
{
  _datagram.xtc.alloc(fragment.size());
  _stream.insert(fragment,fragment.size());
}

inline Pds::ZcpDatagram::ZcpDatagram(const TypeId& type, const Src& src) :
  _datagram(type,src)
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
/*
inline int Pds::ZcpDatagram::_insert(char* p, int size)
{
  int len = _stream.insert(p,size);
  _datagram.xtc.alloc(len);
  return len;
}
*/
inline int Pds::ZcpDatagram::_insert(ZcpFragment& zf, int size)
{
  int len = _stream.insert(zf,size);
  _datagram.xtc.alloc(len);
  return len;
}

inline int Pds::ZcpDatagram::_insert(ZcpStream& s, int size, ZcpFragment& f)
{
  int remaining = size;
  while(remaining) {
    int len = s.remove(f,remaining);
    _stream.insert(f,len);
    remaining -= len;
  }
  _datagram.xtc.alloc(size);
  return size;
}

inline int Pds::ZcpDatagram::dump() const
{
  return _stream.dump();
}
#endif
