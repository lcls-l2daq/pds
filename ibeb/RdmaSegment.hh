#ifndef Pds_IbEb_RdmaSegment_hh
#define Pds_IbEb_RdmaSegment_hh

#include "pds/ibeb/RdmaBase.hh"

#include "pds/ibeb/RdmaWrPort.hh"
#include "pds/ibeb/ibcommon.hh"
#include "pds/service/RingPool.hh"

#include <list>
#include <vector>
#include <stdint.h>

namespace Pds {
  class Datagram;
  namespace IbEb {
    class RdmaSegment : public RdmaBase {
    public:
      RdmaSegment(unsigned   poolSize,
                  unsigned   maxEventSize,
                  unsigned   index,
                  unsigned   numEbs);
      ~RdmaSegment();
    public:
      static void verbose(bool);
    public:
      unsigned nBuffers() const;
      int      fd(unsigned i) const { return _ports[i]->fd(); }
    public:
      void* alloc(unsigned  minSize);
      void queue (unsigned  dst, 
                  unsigned  tgt, 
                  Datagram* p,
                  size_t    psize);
      void dequeue(const RdmaComplete&);
    private:
      bool _alloc  (unsigned eb, 
                    unsigned buf);
      void _dealloc(unsigned eb, 
                    unsigned buf);
      void req_write(unsigned  eb,
                     unsigned  index, 
                     Datagram* dg);
    private:
      RingPool*             _pool;
      unsigned              _src;
      unsigned              _wr_id;
      ibv_mr*               _mr;
      std::vector<RdmaWrPort*> _ports;
      unsigned              _elemSize;
      uint32_t*             _buff;
      std::list<unsigned>   _dqueue;
      std::list<unsigned>   _tqueue;
      std::list<Datagram*>  _pqueue;
    };
  };
};

inline void* Pds::IbEb::RdmaSegment::alloc(unsigned minSize)
{
  return _pool->alloc(minSize);
}

#endif
