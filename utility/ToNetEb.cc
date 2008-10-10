#include <sys/uio.h>
#include <string.h>

#include "ToNetEb.hh"
#include "OutletWireHeader.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagram.hh"
#include "Mtu.hh"
#include "ChunkIterator.hh"

using namespace Pds;

ToNetEb::ToNetEb(int interface, 
		 unsigned payloadsize, 
		 unsigned maxdatagrams) :
  _client(sizeof(OutletWireHeader), payloadsize, Ins(interface), 
	  maxdatagrams)
{  
  if (_client.error()) {
    printf("*** ToNetEb ctor error: %s\n", strerror(_client.error()));
  }
}

/*
** ++
**
**   Transmits the datagram specified by its argument ("datagram" to the
**   address specified as an argument ("dst"). Datagrams are actually
**   transmitted on the wire in units of "Mtu::Size". If the datagram is
**   greater than this value it is broken into a series of fragments,
**   each transmitted individually on the wire. If this is the case, the
**   datagram header (for each fragment) carries not only the size of
**   the complete datagram, but its relative offset within the datagram.
**   This information allows its partner event builder to reassemble
**   the sent fragments back into whole datagrams (see "EbSegment" and
**   "EbEvent").
**   The function returns the status of the transmission operation. A value
**   of zero (0) indicates success. A non-zero value indicates a transmission
**   failure and corresponds to the ERRNO value as returned by the underlying
**   TCP/IP network services.
**
**   NOTE: Any changes made here should be reflected in
**   ToNetEb::_send(const Datagram*, const char*, const Ins&).
** --
*/

int ToNetEb::send(const CDatagram* cdatagram,
		  const Ins&      dst)
{
  const Datagram& datagram = cdatagram->datagram();
  unsigned size = datagram.xtc.extent;

  if(size <= Mtu::Size)  return _client.send((char*)&datagram,
					     (char*)&datagram.xtc,
					     size,
					     dst);

  DgChunkIterator chkIter(&datagram);

  do {
    int error;
    if((error = _client.send((char*)chkIter.header(), 
			     (char*)chkIter.payload(), 
			     chkIter.payloadSize(), 
			     dst)))
      return error;
  } while(chkIter.next());

  return 0;
}

/*
** ++
**
**   Similar to the above _send method, but sends data described by an array
**   of iovec entries, instead.
**
**   I (RiC) don't know why Mike designed it like this, but I followed his
**   example by passing the datagram to the send routine separately.  This
**   means that the first entry in the iovec array is replaced by what was
**   already there in Client, presuming Client::sizeofDatagram() returns
**   the same value as sizeof(Datagram).  It looks like they're always
**   the same, so I don't understand why he didn't use the constant rather
**   than causing a memory fetch.
**
**   As soon as a piece of the array has been sent we can scribble over the
**   sent entries.  Thus, we loop over entries and sum up the sizes.  When it
**   becomes bigger than the allowed size, we truncate and remember the
**   remainder.  After the send, we modify the previous entry that was sent to
**   start at the truncation point.  Then repeat until all chunks are sent.
**
** --
*/

int ToNetEb::send(ZcpDatagram* zdatagram,
		  const Ins&   dst)
{
  const Datagram& datagram = zdatagram->datagram();
  unsigned remaining = datagram.xtc.sizeofPayload();

  if (!remaining) return _client.send((char*)&datagram,
				      (char*)&datagram.xtc,
				      sizeof(InXtc),
				      dst);

  int size = zdatagram->_stream.remove(_fragment, remaining);
  remaining -= size;

  if (!remaining) return _client.send((char*)&datagram,
				      (char*)&datagram.xtc,
				      sizeof(InXtc),
				      _fragment,
				      size,
				      dst);

  OutletWireHeader header(&const_cast<Datagram&>(zdatagram->datagram()));
  int error = _client.send((char*)&header,
			   (char*)&datagram.xtc,
			   sizeof(InXtc),
			   _fragment,
			   size,
			   dst);
  if(error)  return error;
  
  size += sizeof(InXtc);

  while(remaining) {
    header.offset += size;
    size = zdatagram->_stream.remove(_fragment, remaining);
    remaining -= size;
    error = _client.send((char*)&header,
			 _fragment,
			 size,
			 dst);
    if(error)  return error;
  }

  return 0;
}

