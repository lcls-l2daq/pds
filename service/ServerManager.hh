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

#ifndef PDS_SERVERMANAGER
#define PDS_SERVERMANAGER

#include "Select.hh"
#include "Server.hh"
#include "NetServer.hh"
#include "SelectManager.hh"

namespace Pds {

class Ins;

class ServerManager : public NetServer,  // some concrete OobServer
		      public Select, 
		      public SelectManager<Server> {
public:
  ServerManager(const Ins& ins,
		int           sizeofDatagram,
		int           maxPayload,
		unsigned      timeout,
		int           maxDatagrams = 40);
  ServerManager(int           sizeofDatagram,
		int           maxPayload,
		unsigned      timeout,
		int           maxDatagrams = 40);
  virtual ~ServerManager() {}

  void dotimeout(unsigned timeout);
  void dotimeout();
  void donottimeout();

  // Implements Select
  virtual int poll();

private:
  struct timeval  _tmoBuffer;    // Timeout definition for "select" 
  struct timeval* _tmoptr;       // Pointer to tmo struct (can be null)
};
}
#endif
