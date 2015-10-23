#ifndef PDS_OOBSERVER_HH
#define PDS_OOBSERVER_HH

#include "Server.hh"

#include "LinkedList.hh"

namespace Pds {

  class OobServer : public Server 
  {
  public:
    virtual ~OobServer() {}
  public:
    OobServer() {}
    OobServer(unsigned id, int fd) { Server::id(id); Server::fd(fd); }
  public:
    virtual int unblock(char* datagram) = 0;
    virtual int unblock(char* datagram, char* payload, int size) = 0;
  };

}

#endif
