#ifndef PDS_ZCPEBEVENT_HH
#define PDS_ZCPEBEVENT_HH

#include "EbEventBase.hh"
#include "ZcpEbSegment.hh"

namespace Pds {

class EbEventKey;
class EbServer;
class ZcpFragment;
class ZcpDatagram;
class InDatagram;

class ZcpEbEvent : public EbEventBase
  {
  public:
    void* operator new(size_t, Pool*);
    void  operator delete(void* buffer);
  public:
    ZcpEbEvent();
    ZcpEbEvent(EbBitMask creator, EbBitMask contract, ZcpDatagram*, EbEventKey*);
   ~ZcpEbEvent();

    ZcpDatagram*  zdatagram() const;
  public:
    bool          consume(EbServer*, const EbBitMask&, ZcpFragment&);
    InDatagram*   finalize();
  public:
    unsigned      fixup(const Src&, const EbBitMask&, const TypeId&, ZcpFragment&);
  private:
    ZcpDatagram*  _zdatagram;
    LinkedList<ZcpEbSegment> _pending;       // Listhead, Segments pending fragments
    char*         _segments;      // Next segment to allocate
    char          _pool[sizeof(ZcpEbSegment)*32]; // buffer for segments
  };
}

/*
** ++
**
**    To speed up the allocation/deallocation of events, they have their
**    own specific "new" and "delete" operators, which work out of a pool
**    of a fixed number of fixed size buffers (the size is set to the size
**    of this object). The pool is established by "Eb" and events are
**    allocated/deallocated by "EbServer".
**
** --
*/

inline void* Pds::ZcpEbEvent::operator new(size_t size, Pds::Pool* pool)
{
  return pool->alloc(size);
}

/*
** ++
**
**    To speed up the allocation/deallocation of events, they have their
**    own specific "new" and "delete" operators, which work out of a pool
**    of a fixed number of fixed size buffers (the size is set to the size
**    of this object). The pool is established by "Eb" and events are
**    allocated/deallocated by "EbServer".
**
** --
*/

inline void Pds::ZcpEbEvent::operator delete(void* buffer)
{
  Pds::Pool::free(buffer);
}

/*
** ++
**
**    As soon as an event becomes "complete" its datagram is the only
**    information of value within the event. Therefore, when the event
**    becomes complete it is deleted which cause the destructor to
**    remove the event from the pending queue.
**
** --
*/

inline Pds::ZcpEbEvent::~ZcpEbEvent()
{
}


inline Pds::ZcpDatagram* Pds::ZcpEbEvent::zdatagram() const
{
  return _zdatagram;
}

#endif
