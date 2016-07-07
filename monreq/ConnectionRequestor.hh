#ifndef Pds_MonReq_ConnectionRequestor_hh
#define Pds_MonReq_ConnectionRequestor_hh

#include "pds/monreq/ConnInfo.hh"
#include "pds/service/Sockaddr.hh"
#include "pds/service/Ins.hh"

namespace Pds {
  namespace MonReq {

    class ConnectionRequestor {
    public:
      ConnectionRequestor() {}
      //
      //  Open a UDP socket on "interface"
      //  Will request connections to "nodes" on TCP "interface","port"
      //
      ConnectionRequestor(int interface, unsigned nodes, Ins mcast);

    public:
      int socket();
      //
      //  Receive requested connections
      //
      int receiveConnection();
      //
      //  Send out request for connections from "nodes"
      //
      int requestConnections();

    private:
      int      _listen_socket;
      int      _socket;   // Descriptor of udp sending socket
      Sockaddr _mcast;
      ConnInfo _info;     // Connection request message
    };

  }
}
#endif
