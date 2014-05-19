/*
** ++
**  Package:
**	Service
**
**  Abstract:
**      non-inline functions for class "SelectManager"
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

#include "SelectManager.hh"
#include <string.h>
#include <stdio.h>

using namespace Pds;

/*
** ++
**
**   The constructor for this class accepts as arguments the length of time
**   (in 10 millisecond tics) to time-out a pending "select" (see "poll")
**   and the arguments necessary to construct the server which this class
**   inherits from. These arguments include: its own address "ins", the size 
**   of the datagram header "sizeofDatagram" (in bytes), the maximum payload
**   the server can receive (in bytes),  and a reference to a longword into
**   which the construction status of the server is to be returned (a value
**   of zero is success). This base class specifies a server to be used to 
**   handle Out-Of-Band messages. Any messages delivered to ourselves will be
**   serialized with the other servers managed by this facility, however
**   the return value from our "pend" function will be used to decide whether 
**   control will be returned to the caller of the class's "poll" function.
**   In addition to setting up the "select" timeout, the constructor set the 
**   database to empty and initializes the select's file descriptor map to 
**   empty.
**
** --
*/

template<class T>
SelectManager<T>::SelectManager(OobServer& oobServer, unsigned timeout) :
  _timeout(timeout),
  _oobServer(oobServer)
  {
  _managedList.clearAll();
  _activeList .clearAll();
  _ioList            = (char*)&_ioListBuffer;

  memset(&_ioListBuffer, 0, sizeof(_ioListBuffer));

  _managed.insert(&oobServer);
  enable(&oobServer);
  }

/*
** ++
**
**   The constructor for this class accepts as arguments the length of time
**   (in 10 millisecond tics) to time-out a pending "select" (see "poll")
**   and the arguments necessary to construct the server which this class
**   inherits from. These arguments include: the size of the datagram header
**   "sizeofDatagram" (in bytes), the maximum payload the server can receive
**   "maxPayload" (in bytes). This base class specifies a server to be used to 
**   handle Out-Of-Band messages. Any messages delivered to ourselves will be
**   serialized with the other servers managed by this facility, however
**   the return value from our "pend" function will be used to decide whether 
**   control will be returned to the caller of the class's "poll" function.
**   In addition to setting up the "select" timeout, the constructor set the 
**   database to empty and initializes the select's file descriptor map to 
**   empty.
**
** --
*/

template<class T>
int SelectManager<T>::manage(T* server)
{
  EbBitMask id; 
  id.setBit(server->id());
  EbBitMask managedList = _managedList;

  if((id & managedList).isNotZero()) return 0;

  T*  managed = _managed.forward();
  int socket  = server->fd();

  while(managed != _managed.empty())
    {
    if(managed->fd() == socket) return 0;  
    if(managed->fd() >  socket) break;  
    managed = managed->forward();
    }

  server->connect(managed->reverse());

  _servers[server->id()] = server;
  _managedList           = managedList | id;

  return 1;
}

/*
** ++
**
**    This function will arm a previously managed server. A server which is
**    armed will process any datagrams received (see the "poll" function)
**    Note, that this function is non-reentrant and should not be called 
**    asynchronously if a "select" is pending (see "poll"). One safe way to 
**    call this function, if waiting, is to call this function from within 
**    a server managed by this class (In particular the Out-Of-Band server). 
**    Since I/O from a server is processed one server at a time,this will 
**    guarantee serialization.If the server is already armed, the function 
**    will return a value of zero (0), otherwise one (1).
**
** --
*/

template<class T>
int SelectManager<T>::arm(T* server)
{
  EbBitMask id;
  id.setBit(server->id());
  EbBitMask active = _activeList;

  if((_managedList & id).isZero()) return 0;

  _activeList = active | id;

  enable(server);

  return 1;
  }

/*
** ++
**
**    This function will arm ALL the currently managed servers. It will
**    return the list (as a bit list) of the servers currently managed.
**    If a server is already armed it will remain armed.
**
** --
*/

template<class T>
EbBitMask SelectManager<T>::arm(EbBitMask mask)
  {
  EbBitMask    id(EbBitMask::ONE);
  T**             server    = &_servers[0]; 
  EbBitMask    managed   = _managedList & mask;
  EbBitMask    remaining = managed;

  _activeList = remaining;

  do
    {
    if((id & remaining).isZero()) continue;
    enable(*server);
    remaining &= ~id;
    }
  while(server++, (id <<= 1).isNotZero(), remaining.isNotZero());

  return managed;
  }

/*
** ++
**
**    This function will make "safe" a previously managed and armed server. A 
**    server which is armed will process any datagrams received (see the 
**    "poll" function). Note, that this function is non-reentrant and should 
**    not be called asynchronously if a "select" is pending (see "poll"). One 
**    safe way to call this function, if waiting, is to call this function 
**    from within a server managed by this class (In particular the Out-Of-
**    Band server). Since I/O from a server is processed one server at a time,
**    this will guarantee serialization.If the server is already armed, the 
**    function will return a value of zero (0), otherwise one (1).
**
**
** --
*/

template<class T>
EbBitMask SelectManager<T>::safe(T* server)
{
  EbBitMask id;
  id.setBit(server->id());
  EbBitMask active = _activeList;
  EbBitMask on     = active & id;

  if(on.isNotZero())
    {
    _activeList = active & ~id;
    disable(server);
    }

  return on;
  }

/*
** ++
**
**
** --
*/

template<class T>
T* SelectManager<T>::unmanage(T* thisServer)
{
  T* server = thisServer;
  
  if(server)
    {
      EbBitMask id;
      id.setBit(server->id());
      EbBitMask managed = _managedList;
      if((managed & id).isNotZero())
	{
	  _managedList = managed & ~id;
	  safe(server);
	  server->disconnect();
	}
    }
  
  return server;
}

/*
** ++
**
**    This is an internal routine, whose function is to first, check for
**    I/O on the ourselves and if so, call back its "pend" function to
**    discover whether to abort the pend or not. Second, for each server
**    armed within the database, check whether it has I/O pending and if 
**    so call back the manager's "processIo" function to process it. The 
**    "processIo" function is passed a pointer to the active server as an 
**    argument. This function returns a non-zero value if the server is to
**    be re-armed and a zero value if it is to be not re-armed.
**
** --
*/

template<class T>
int SelectManager<T>::_dispatchIo()
{
  EbBitMask   remaining = _activeList;
  if(enable(&_oobServer))
    {
    if(!_oobServer.pend())
      {
      return 0;
      }
    }
  EbBitMask active    = _activeList;
  EbBitMask id(EbBitMask::ONE);
  T**  next    = _servers;
  T*   server;

  remaining &= active; // some servers may have been deleted

  do
    {
    if((id & remaining).isZero()) continue;
    server = *next;    
    if(disable(server))
      {
      if(!processIo(server))
        {
        active &= ~id;
        }
      else
        enable(server);
      }
    else
      enable(server);
    remaining &= ~id;
    }
  while(next++, (id <<= 1).isNotZero(), remaining.isNotZero());

  _activeList = active;

  return 1;
}

template<class T>
void SelectManager<T>::dump() const
{
  printf("  active : "); active ().print(); printf("\n");
  printf("  managed: "); managed().print(); printf("\n");
  printf("  fds    : ");
  EbBitMask m = managed();
  for(unsigned i=0; !m.isZero(); i++) {
    Server* s = server(i);
    if (s) {
      printf(" %d",s->fd());
      m.clearBit(i);
    }
  }
  printf("\n");
      
  unsigned n = numFds();
  printf("  isset: ");
  for(unsigned i=0; i<n; i++)
    printf("%c",FD_ISSET(i,ioList()) ? '+':'_');
  printf(": %08x %08x\n", ((unsigned*)ioList())[0],((unsigned*)ioList())[1]);
}
