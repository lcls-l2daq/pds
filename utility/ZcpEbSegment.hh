#ifndef Pds_ZcpEbSegment_hh
#define Pds_ZcpEbSegment_hh

#include "pds/service/LinkedList.hh"
#include "Mtu.hh"
#include "pds/xtc/xtc.hh"
#include "pds/service/EbBitMask.hh"
#include "pds/service/ZcpFragment.hh"

typedef unsigned size_t;

namespace Pds {

class ZcpEbSegment : public LinkedList<ZcpEbSegment>
  {
  public:
    void* operator new(size_t, char**);
  public:
    ZcpEbSegment(const InXtc& header,
		 int offset,
		 int length,
		 EbBitMask client,
		 ZcpFragment*,
		 ZcpEbSegment* pending);
    ZcpEbSegment(ZcpEbSegment&);
  public:
   ~ZcpEbSegment() {}
  public:
    ZcpEbSegment* consume  (ZcpFragment*,
			    int offset);
    int           remaining() const;
    EbBitMask     client   () const;
    bool          fixup    (const Src&, const TC&);
    LinkedList<ZcpFragment>& fragments();
  private:
    int          _offset;
    int          _remaining;
    EbBitMask    _client;
    InXtc        _header;
    LinkedList<ZcpFragment> _fragments;
  };
}

/*
** ++
**
**    To speed up the allocation of segments, they have their own specific
**    "new" operator, which work out of a fixed size buffer allocated for
**    each event (see "EbEvent"). Since the number of segments per
**    event is limited to the maximum number of contributors per event, a
**    fixed-size buffer is appropriate and explains the lack of bounds checking.
**    Note, the absence of a corresponding "delete" operator (much to the
**    horror of politically correct OOPs guys). In actual fact, the memory
**    is returned when the event containing the buffer is deleted.
**
** --
*/

inline void* Pds::ZcpEbSegment::operator new(size_t size, char** next)
  {
  char* buffer = *next;
  *next = buffer + sizeof(Pds::ZcpEbSegment);
  return (void*)buffer;
  }

/*
** ++
**
**    Returns the number of bytes remaining to satisfy the segment.
**
** --
*/

inline int Pds::ZcpEbSegment::remaining() const
  {
  return _remaining;
  }

/*
** ++
**
**    Returns the client who is to provide the data for the segment.
**
** --
*/

inline EbBitMask Pds::ZcpEbSegment::client() const
  {
  return _client;
  }


inline Pds::LinkedList<Pds::ZcpFragment>& Pds::ZcpEbSegment::fragments()
{
  return _fragments;
}

#endif
