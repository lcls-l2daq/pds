/*
** ++
**  Package:
**	odfUtility
**
**  Abstract:
**	Non-inline functions for class "odfEbServer"
**
**  Author:
**      Michael Huffer, SLAC, (415) 926-4269
**
**  Creation Date:
**	000 - June 1,1998
**
**  Revision History:
**	None.
**
** --
*/

#include "EbServer.hh"
#include "Mtu.hh"
#include "OutletWireHeader.hh"

using namespace Pds;

/*
** ++
**
**    This function instantiates a server. A server is created whenever
**    a client upstream of the server comes into existence. I.e., for
**    each client, their exists one and only one server. The client
**    has a identity ("id"), which is passed as an argument to the
**    constructor. This ID is a small dense integer varying (currently)
**    from zero to thirty-one (decimal), used to uniquely identify the
**    client. Within, the server the ID is represented as a bit mask,
**    allowing the object's which use the ID to operate more efficiently.
**    The constructor is passed two fixed-length pools:
**    "datagrams", which serves as a freelist for event buffering, and
**    "events" which serves as a freelist for the objects which describe
**    the context necessary to construct an event. The "pending" argument
**    is the list of events currently under either construction or completion.
**    The queue is in arrival order, with the oldest event at the head and
**    the youngest event at the tail. Since the socket paradigm forces the
**    implementation to allocate one ahead (the read interface requires
**    a buffer to read into), an event may be under "construction", which
**    means the event has been allocated but has not yet seen an actual
**    contribution. An event which has seen at least one contribution, but
**    not all its necessary contributions is said to be under "completion".
**    The "clients" argument points to the list of all currently known
**    clients. This function saves all the arguments and locates the event
**    to contain its first seen contribution.
**
**
** --
*/

EbServer::EbServer(const Src& client, unsigned id, int fd) :
  Server(id,fd),
  _keepAlive(KeepAlive),
  _drop(0),
  _client(client)
{
}

/*
** ++
**
**   This function is NOT used by this server, as its manager ("Eb"),
**   allocates memory for each server directly.
**
** --
*/

char* EbServer::payload()
{
  return (char*)0;
}

/*
** ++
**
**   This function is NOT used by this server, as its manager ("Eb"),
**   proccess received contributions from server directly.
**
** --
*/

int EbServer::commit(char* key,
                        char* payload,
                        int sizeofPayload,
                        const Ins&)
{
  return 1;
}

/*
** ++
**
**    This function is called back whenever the read posted by the server
**    encounters an error. The argument passed to this function is the
**    "errno" value corresponding to the reason the read failed. Since I
**    haven't figured out what to do with this yet, the function simply
**    returns...
**
** --
*/

#include <string.h>

int EbServer::handleError(int value)
{
  printf("*** EbServer::handleError pid %04X error: %s\n",
         _client.pid(), strerror(value));
  return 1;
}

/*
** ++
**
**    Destructor for server.
**
** --
*/

EbServer::~EbServer()
{
}
