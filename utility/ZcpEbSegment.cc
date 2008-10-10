/*
** ++
**  Package:
**	odfService
**
**  Abstract:
**	Non-inline member functions for class "odfZcpEbSegment"
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

#include "ZcpEbSegment.hh"
#include "pds/service/ZcpFragment.hh"
#include "pds/xtc/xtc.hh"

#include <stdio.h>

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

ZcpEbSegment::ZcpEbSegment(const InXtc&  header,
                           int           offset,
                           int           length,
                           EbBitMask     client,
			   ZcpFragment&  zfrag,
                           ZcpEbSegment* pending) :
  _offset(offset+zfrag.size()),
  _length(length),
  _remaining(length-zfrag.size()),
  _client(client),
  _header(header)
{
  _header.alloc(header.sizeofPayload());
  _fragments.insert(zfrag,zfrag.size());
  pending->insert(this);
}

ZcpEbSegment::~ZcpEbSegment()
{
  disconnect();
}

/*
** ++
**    This function is called for each arrived fragment within a segment
**    (with the exception of the first fragment, which is handled by the
**    constructor described above).  We only accept fragments that arrive
**    in order (offset equals the expected value).
** --
*/

ZcpEbSegment* ZcpEbSegment::consume(ZcpFragment& zfrag,
				    int          offset)
{
  if ( offset != _offset ) {
    zfrag.flush();
    return 0;
  }

  _offset    += zfrag.size();
  _remaining -= zfrag.size();
  _fragments.insert(zfrag,zfrag.size());

  return _remaining ? 0 : this;
}

