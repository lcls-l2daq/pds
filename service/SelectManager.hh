/*
** ++
**  Package:
**	Service
**
**  Abstract:
**      This class has serves two major functions. First it provides a 
**      database (with all its attendant functions) to manage a collection
**      of servers. For example, "manage" and "umanage" to add or subtract
**      servers from the database. Second, using this database as input,
**      the class provides a wrapper around the "select" I/O service, 
**      allowing the user to multiplex reads from more than a single server 
**      AND to timeout the wait on any single read (see "pend" and the 
**      virtual functions "processIo" and "processTmo"). Note: that all
**      of the database functionality is NOT reentrant. Therefore, none of
**      database related functions should be called if the class is waiting.
**      The pattern to get around this problem, is to run these functions
**      within a managed server. Since only one server runs at a time, this
**      will serialize access to the database.
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

#ifndef PDS_SELECT_MANAGER
#define PDS_SELECT_MANAGER

#ifndef VXWORKS
#include <sys/time.h>
#endif

#include "LinkedList.hh"
#include "EbBitMask.hh"
#include "OobServer.hh"

namespace Pds {
template<class T> class SelectManager
{
public:
  SelectManager(OobServer& oobServer, unsigned timeout);
  virtual ~SelectManager() {}

public:
  EbBitMask arm(EbBitMask mask = EbBitMask(EbBitMask::FULL));
  int          arm     (T*);
  int          manage  (T*);
  EbBitMask safe    (T*);
  T*           unmanage(T*);

  EbBitMask active()  const {return _activeList;}
  EbBitMask managed() const {return _managedList;}
  unsigned     timeout() const {return _timeout;}

  T* server(unsigned id) {return (_managedList.hasBitSet(id)) ? _servers[id] : 0;}
  T** table() {return &_servers[0];}

  virtual int processIo(T*) = 0;
  virtual int processTmo()  = 0;

protected:
  unsigned enable (T*);
  unsigned disable(T*);
  OobServer& oobServer() {return _oobServer;}
  int numFds() const;
  fd_set* ioList() {return (fd_set*)_ioList;}
  int _dispatchIo();

private:
  LinkedList<T> _managed;      // Listhead of managed servers
  EbBitMask     _managedList;  // Bit-list of managed servers (by ID)
  EbBitMask     _activeList;   // Bit-list of active servers (by ID)
  char*            _ioList;       // -> Database of "live" file descrs
  fd_set           _ioListBuffer; // Database of "live" file descrs
  unsigned         _timeout;      // Timeout definition for "select" 
  T*               _servers[EbBitMask::BitMaskBits];  // Lookup table of servers (by ID)
  OobServer&       _oobServer;
};
}
/*
** ++
**
**    This function is used in conjunction with the UNIX IO "select"
**    mechanism (see "SelectManager") and is used to enable the
**    server's socket for the select table specified as an argument
**    to this function. The function returns the server's previous 
**    socket state. A positive (non-zero) value indicates the socket 
**    WAS enabled, a zero (0) value indicates the socket was disabled.
**
** --
*/

template<class T>
inline unsigned Pds::SelectManager<T>::enable(T* server)
  {
  unsigned  socket  = server->fd();
  unsigned* base    = (unsigned*)((socket >> 5 << 2) + _ioList);
  unsigned  mask    = 0x1 << (socket & 0x1f);
  unsigned  current = *base;
  
  *base = current | mask;
  
  return current & mask;
  }
  

/*
** ++
**
**    This function is used in conjunction with the UNIX IO "select"
**    mechanism (see "SelectManager") and is used to disable the
**    server's socket for the select table specified as an argument
**    to this function. The function returns the server's previous 
**    socket state. A positive (non-zero) value indicates the socket 
**    WAS enabled, a zero (0) value indicates the socket was disabled.
**
** --
*/

template<class T>
inline unsigned Pds::SelectManager<T>::disable(T* server)
  {
  unsigned  socket  = server->fd();
  unsigned* base    = (unsigned*)((socket >> 5 << 2) + _ioList);
  unsigned  mask    = 0x1 << (socket & 0x1f);
  unsigned  current = *base;

  *base = current & ~mask;

  return current & mask;
  }

template<class T>
inline int Pds::SelectManager<T>::numFds() const
{ 
  T* server = _managed.reverse();
  if (server != _managed.empty())
    return server->fd()+1;
  return 0;
}

#endif
