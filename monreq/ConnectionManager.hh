#ifndef Pds_MonReq_ConnectionManager_hh
#define Pds_MonReq_ConnectionManager_hh

#include <vector>
#include "pds/monreq/ServerConnection.hh"

namespace Pds {
class Ins;
  namespace MonReq {

    class ConnectionManager {
    public:
      //
      //  Open a UDP socket on "interface" and join multicast group "mcast"
      //
      ConnectionManager(int interface, const Ins& mcast, int id, std::vector<ServerConnection>& servers);

    public:
      //
      //  File descriptor for "polling" on received connection requests
      //
      int socket() const;
      //
      //  Receive multicast connection request and return connected socket descriptor
      //
      int receiveConnection();

    private:
      int _socket;   // Descriptor for mcast listening socket
      int _id;
      std::vector<ServerConnection>& _servers;
    };

  }
}
#endif
