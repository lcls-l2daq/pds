#ifndef Pds_MonReq_ServerConnection_hh
#define Pds_MonReq_ServerConnection_hh

namespace Pds {
  class Datagram;

  namespace MonReq {

    class ServerConnection {
    public:
      //
      //  Track the connection to a requestor on TCP "socket"
      //
      ServerConnection(int socket);

      ServerConnection() {}

      int address() const;

    public:
      //
      //  Return the socket number for setting up poll
      //
      int socket() const;

      //
      //  Receive a new request
      //
      int recv();

      //
      //  Returns number of requests sent
      //
     // int NumSent() const;

      //
      //  Fulfill an outstanding request, if necessary
      //
      int send(const Datagram&);

    private:
      int _socket;   // Descriptor for connected TCP socket
      int _address;
      int _numberRequested;
      int _numberSent;
    };

  }
}
#endif
