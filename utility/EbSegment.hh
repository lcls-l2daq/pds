/*
** ++
**  Package:
**	odfService
**
**  Abstract:
**      This class is used to describe the various data-fragments which
**      consitute a segment. A segment is a single logical contiguous
**      piece of data transported by any single contributor to the
**      event builder. These contributors can only transport such logical
**      blocks in dis-contiguous pieces quantized in units of MTU (Maximum
**      Transport Units). This class is used to manage the re-assembly of
**      these arriving fragments into a contiguous whole. To do, the class
**      maintains state of the address of the segment, how many fragments
**      remain to form the segment, and where the next fragment should be
**      located relative to its starting address. Finally, since fragments
**      may NEVER materialize to satisfy the segment, the class allows itself
**      to be inserted on a list of pending fragments, thus allowing the
**      assembly process to be timed-out.
**
**  Author:
**      Michael Huffer, SLAC, (415) 926-4269
**
**  Creation Date:
**	000 - September 23,1998
**
**  Revision History:
**	None.
**
** --
*/

#ifndef PDS_EBSEGMENT
#define PDS_EBSEGMENT

#include "pds/service/LinkedList.hh"
#include "Mtu.hh"
#include "pds/xtc/xtc.hh"
#include "pds/service/EbBitMask.hh"

typedef unsigned size_t;

namespace Pds {
class EbSegment : public LinkedList<EbSegment>
  {
  public:
    void* operator new(size_t, char**);
  public:
    EbSegment(const InXtc& header,
	      char* base,
	      int size,
	      int offset,
	      int length,
	      EbBitMask client,
	      EbSegment* pending);
  public:
   ~EbSegment() {}
  public:
    EbSegment* consume(int sizeofFragment, int offsetExpected);
    char*         payload();
    char*         base();
    int           remaining() const;
    EbBitMask  client() const;
    void          fixup(const TC&);
    EbSegment* notComplete();
  private:
    char*        _base;
    int          _offset;
    int          _remaining;
    EbBitMask _client;
    InXtc     _header;
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

inline void* Pds::EbSegment::operator new(size_t size, char** next)
  {
  char* buffer = *next;
  *next = buffer + sizeof(Pds::EbSegment);
  return (void*)buffer;
  }

/*
** ++
**
**   Returns the (anticipated) location of the memory for the fragment
**   next to arrive.
**
** --
*/

inline char* Pds::EbSegment::payload()
  {
  return _base + _offset;
  }

/*
** ++
**
**   Returns the starting address of the segment.
**
** --
*/

inline char* Pds::EbSegment::base()
  {
  return _base;
  }

/*
** ++
**
**    Returns the number of bytes remaining to satisfy the segment.
**
** --
*/

inline int Pds::EbSegment::remaining() const
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

inline EbBitMask Pds::EbSegment::client() const
  {
  return _client;
  }

/*
** ++
**
**    If the segment is not completed then a pointer to this segment is
**    returned, otherwise a NULL pointer is returned.
**
** --
*/

inline Pds::EbSegment* Pds::EbSegment::notComplete(){
  if(!_remaining)
    {
    disconnect();
    return (Pds::EbSegment*)0;
    }
  else if (_offset >= (int)(_header.sizeofPayload()+sizeof(InXtc))) 
    {
    fixup(_header.tag);
    disconnect();
    return (Pds::EbSegment*)0;
    }
  else
    return this;
}

#endif
