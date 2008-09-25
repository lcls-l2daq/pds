/*
** ++
**  Package:
**	odfService
**
**  Abstract:
**	Non-inline member functions for class "odfEbSegment"
**
**  Author:
**      Michael Huffer, SLAC, (415) 926-4269
**
**  Creation Date:
**	000 - November 3,1998
**
**  Revision History:
**	None.
**
** --
*/

#include "EbSegment.hh"
#include "pds/xtc/xtc.hh"

using namespace Pds;

/*
** ++
**
**    The constructor for a section descriptor. This function is called AFTER
**    the first fragment for the section arrives. Consequently, the memory
**    for the section has already been allocated (as specified by the "base"
**    argument) and the fragment whose arrival triggered this member function
**    is assumed to start at offset zero (0). The actual offset at which the
**    fragment is to be located is passed as an argument ("offset"). If the
**    fragment should not be located at zero, it is relocated to its correct
**    offset. The next fragment is then assumed to locate at the location
**    following the just arrived fragment. Note, that this function
**    initializes the object's internal state which includes the number
**    of bytes remaining to satisfy the segment. Last, the object is inserted
**    on a list of pending segments (whose list-head is specified by the
**    "pending" argument, in order to allow timing out the non-arrival of
**    fragments.
**
** --
*/

EbSegment::EbSegment(const InXtc& header,
		     char* base,
		     int sizeofFragment,
		     int offset,
		     int length,
		     EbBitMask client,
		     EbSegment* pending) :
  _base(base),
  _offset(offset + sizeofFragment),
  _remaining(length - sizeofFragment),
  _client(client),
  _header(header)
  {
  pending->insert(this);

  _header.tag.extend(header.sizeofPayload());

  if(offset != 0)
    {
    unsigned remaining = (unsigned)sizeofFragment >> 2;
    unsigned* in       = ((unsigned*) base)           + remaining;
    unsigned* out      = ((unsigned*)(base + offset)) + remaining;
    if(remaining) do *(--out) = *(--in); while(--remaining);
    }
  }

/*
** ++
**
**    This function is called for each arrived fragment within a segment
**    (with the exception of the first fragment, which is handled by the
**    constructor described above). Since it is called after the fragment
**    has arrived the descriptor contains its best guess of where the
**    fragment relative to the segment should actually reside. The actual
**    offset of the fragment is determined by the "expected" argument. If
**    they differ, the fragment data is relocated to its appropriate
**    location. Note, that the relocation code ASSUMES the size of the
**    fragment is an intergral number of longwords (4 byte words), as specified
**    by the "sizeofFragment" argument. The function than adjusts both the
**    number of bytes remaining to satisfy the segment and the location
**    of the next expected fragment. Note: that since fragments are
**    transmitted in order, the next fragment is expected (but not required)
**    to follow the fragment just processed. If the segment has been satisfied
**    the object is removed from its pending list and a sentinal value of
**    NIL (zero) is returned. If the arrived fragment does not satisfy the
**    segment it returns a pointer to itself.
**
** --
*/

EbSegment* EbSegment::consume(int sizeofFragment, int expected)
  {
  int offset = _offset;

  if(offset != expected)
    {
    char* base         = _base;
    unsigned* in       = (unsigned*)(base + sizeofFragment + offset);
    unsigned* out      = (unsigned*)(base + sizeofFragment + expected);
    unsigned remaining = (unsigned)sizeofFragment >> 2;
    offset             = expected;
    if(remaining) do *(--out) = *(--in); while(--remaining);
    }

  _offset     = sizeofFragment + offset;
  _remaining -= sizeofFragment;

  return notComplete();
  }

/*
** ++
**
** fixup() is called when a missing "chunk" is identified. The InXtc
** header is faked up to make sure that it wasn't lost/corrupted. The datagram
** is also marked with the apropriate damage.
**
**
** --
*/

void EbSegment::fixup(const TC& dummy){
  Damage damaged(1 << Damage::IncompleteContribution);
  InXtc* inXtc = new(_base) InXtc(dummy, _header.src, damaged);
  inXtc->tag.extend(_header.sizeofPayload());
}
