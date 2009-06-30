#ifndef Pds_ZcpEbSegment_hh
#define Pds_ZcpEbSegment_hh

#include "pds/service/LinkedList.hh"
#include "Mtu.hh"
#include "pds/service/EbBitMask.hh"
#include "pds/service/ZcpStream.hh"
#include "pdsdata/xtc/Xtc.hh"

namespace Pds {

  class ZcpFragment;

  class ZcpEbSegment : public LinkedList<ZcpEbSegment>
  {
  public:
    void* operator new(size_t, char**);
    void  operator delete(void*);
  public:
    ZcpEbSegment(const Xtc&  header,
		 int           offset,
		 int           length,
		 EbBitMask     client,
		 ZcpFragment&  fragment,
		 ZcpEbSegment* pending);
   ~ZcpEbSegment();
  public:
    ZcpEbSegment* consume  (ZcpFragment&,
			    int offset);
    int           remaining() const;
    int           length   () const;
    EbBitMask     client   () const;
    ZcpStream&    fragments();
  private:
    int          _offset;
    int          _length;
    int          _remaining;
    EbBitMask    _client;
    Xtc        _header;
    ZcpStream    _fragments;
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

inline void Pds::ZcpEbSegment::operator delete(void*)
{
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

inline int Pds::ZcpEbSegment::length() const
{
  return _length;
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


inline ZcpStream& Pds::ZcpEbSegment::fragments()
{
  return _fragments;
}

#endif
