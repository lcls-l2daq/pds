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

ZcpEbSegment::ZcpEbSegment(const InXtc&  header,
                           int           offset,
                           int           length,
                           EbBitMask     client,
                           ZcpFragment*  zfrag,
                           ZcpEbSegment* pending) :
  _offset(offset),
  _remaining(length - zfrag->size()),
  _client(client),
  _header(header)
{
  _header.tag.extend(header.sizeofPayload());
  pending->insert(this);
}

/*
**  This is not a copy constructor.
**  Note that the fragments are stolen.
*/
ZcpEbSegment::ZcpEbSegment(ZcpEbSegment& seg) :
  _offset   (seg._offset),
  _remaining(seg._remaining),
  _client   (seg._client),
  _header   (seg._header)
{
  _fragments.insertList(&seg._fragments);
}

/*
** ++
**    This function is called for each arrived fragment within a segment
**    (with the exception of the first fragment, which is handled by the
**    constructor described above).  When a new fragment arrives, there
**    are only two possibilities: the fragments arrive in ascending order
**    or they arrive in descending order; thus it goes either at the 
**    beginning of the list or the end of the list of fragments.
** --
*/

ZcpEbSegment* ZcpEbSegment::consume(ZcpFragment* zfrag,
				    int          offset)
{
  if ( offset < _offset ) {
    LinkedList<ZcpFragment> list;
    list.insert(zfrag);
    list.insert(&_fragments);
    _fragments.insert(&list);
  }
  else
    _fragments.insert(zfrag);

  _offset = offset;
  _remaining -= zfrag->size();

  return _remaining ? 0 : this;
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

bool ZcpEbSegment::fixup(const Src& client,
			 const TC& dummy){
  if (!(client == _header.src))
    return false;

  // rewrite the header of this fragment
  ZcpFragment* hdr = _fragments.forward();

  InXtc localXtc;
  InXtc* xtc;
  if (_offset)
    xtc = &localXtc;
  else
    hdr->uremove(xtc, sizeof(*xtc));

  new((char*)xtc) InXtc( dummy, _header.src, 
			 (1<<Damage::IncompleteContribution) );
  unsigned size = hdr->size();
  xtc->tag.extend(size);
  hdr->uinsert(xtc, sizeof(*xtc));
  hdr->moveFrom(*hdr, size);
  return true;
}
