#ifndef Pds_MonStreamSocket_HH
#define Pds_MonStreamSocket_HH

#include "pds/mon/MonSocket.hh"

namespace Pds {

  class Ins;
  class MonMessage;

  class MonStreamSocket : public MonSocket {
  public:
    MonStreamSocket();
    MonStreamSocket(int socket);
    virtual ~MonStreamSocket();
    
    int fetch  (MonMessage*);
    int readv  (const iovec* iov, int iovcnt);
    int connect(const Ins& dst);
    int listen (const Ins& src);
    int accept();
  };
};

#endif
