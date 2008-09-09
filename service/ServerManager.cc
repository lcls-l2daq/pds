/*
** ++
**  Package:
**	Service
**
**  Abstract:
**      non-inline functions for class "ServerManager"
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

#include "ServerManager.hh"

using namespace Pds;

/*
** ++
**
**   The constructor for this class accepts as arguments the length of time
**   (in millisecond tics) to time-out a pending "select" (see "poll")
**   and the arguments necessary to a reference to the out-of-band server.
**   Any Out-Of-Band messages delivered to the server will be
**   serialized with the other servers managed by this facility, however
**   the return value from the "pend" function will be used to decide whether 
**   control will be returned to the caller of the class's "poll" function.
**   In addition to setting up the "select" timeout, the constructor set the 
**   database to empty and initializes the select's file descriptor map to 
**   empty.
**
** --
*/

ServerManager::ServerManager(const Ins& ins,
			     int           sizeofDatagram,
			     int           maxPayload,
			     unsigned      timeout,
			     int           maxDatagrams) :
  NetServer((unsigned) -1, ins, sizeofDatagram, maxPayload, maxDatagrams),
  SelectManager<Server>(*this, timeout)
  {
    dotimeout(timeout);
  }


ServerManager::ServerManager(int          sizeofDatagram,
			     int          maxPayload,
			     unsigned     timeout,
			     int          maxDatagrams) :
  NetServer((unsigned) -1, sizeofDatagram, maxPayload, maxDatagrams),
  SelectManager<Server>(*this, timeout)
  {
    dotimeout(timeout);
  }

/*
** ++
**
**    This function will wait on the arrival of a datagram to one or more
**    of the server's managed  and armed by this class. Note, that this 
**    includes a datagram destined to the manager itself (i.e, a so-called 
**    Out-Of-Band message). For every datagram received by a managed server, 
**    the "processIo" function is called back, with a pointer to the 
**    appropriate server as an argument. If the message is destined for the 
**    manager itself, its "pend" function is called back. If the value 
**    returned by the "pend" function is zero, the manager is disarmed
**
** --
*/

int ServerManager::poll()
  {
    fd_set* const readfds = ioList();
    fd_set* const writfds = 0;
    fd_set* const excefds = 0;
#ifdef __linux__
    struct timeval tmo;
    tmo.tv_sec  = _tmoBuffer.tv_sec;
    tmo.tv_usec = _tmoBuffer.tv_usec;
    struct timeval* tmoptr = _tmoptr ? &tmo : 0;
    if(select(numFds(), readfds, writfds, excefds, tmoptr) > 0)
#else
    if(select(numFds(), readfds, writfds, excefds, _tmoptr) > 0)
#endif
      {
	return _dispatchIo();  
      }
    else        
      {
	enable(&oobServer());
	return processTmo();
      }
  }

void ServerManager::dotimeout(unsigned timeout) 
{
  _tmoBuffer.tv_usec = (timeout%1000)*1000;
  _tmoBuffer.tv_sec  = timeout/1000;
  _tmoptr = &_tmoBuffer;
}
void ServerManager::dotimeout() {_tmoptr = &_tmoBuffer;}
void ServerManager::donottimeout() {_tmoptr = 0;}
