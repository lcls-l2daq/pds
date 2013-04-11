#include <sys/uio.h>
#include <string.h>

#include "ToNetEb.hh"
#include "OutletWireHeader.hh"
#include "pds/xtc/CDatagram.hh"
#include "Mtu.hh"
#include "ChunkIterator.hh"

#include <stdio.h>

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
    //    printf("ToNetEb::send %d/%d\n",chkIter.payloadSize(),size);
    int error;
    if((error = _client.send((char*)chkIter.header(), 
			     (char*)chkIter.payload(), 
			     chkIter.payloadSize(), 
			     dst)))
      return error;
  } while(chkIter.next());

  return 0;
}
