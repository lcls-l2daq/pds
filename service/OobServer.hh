#ifndef PDS_OOBSERVER_HH
#define PDS_OOBSERVER_HH

#include "Server.hh"

namespace Pds {

  class OobServer : public Server 
  {
  public:
    virtual ~OobServer() {}
  public:
    OobServer(unsigned id, int fd) : Server(id,fd) {}
  public:
    virtual int unblock(char* datagram, char* payload=(char*)0, int size=0) = 0;
  };

}

#endif
