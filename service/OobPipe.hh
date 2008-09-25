#ifndef Pds_OobPipe_hh
#define Pds_OobPipe_hh

#include "OobServer.hh"

namespace Pds {

  class OobPipe : public OobServer {
  public:
    OobPipe(int sizeofDatagram);
    virtual ~OobPipe();
  private:
    virtual char*    payload();
    virtual int      commit(char* datagram,
			    char* payload,
			    int payloadSize);
    virtual int      handleError(int value);
  public:
    int      pend        (int flag = 0);
    int      fetch       (char* payload, int flags);
    int      fetch       (ZcpFragment& , int flags);
  public:
    int unblock(char* datagram);
    int unblock(char* datagram, char* payload, int size);
    int unblock(char* datagram, char* payload, int size, LinkedList<ZcpFragment>&);
  private:
    int    _pfd[2];
    int    _sizeofDatagram;
    char*  _datagram;       // -> buffer for current  datagram
    char** _payload;        // Pointer to -> buffer current payload
  };
}

#endif
