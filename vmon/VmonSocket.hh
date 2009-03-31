#ifndef Pds_VmonSocket_HH
#define Pds_VmonSocket_HH

#include "pds/mon/MonSocket.hh"

#include <sys/types.h>

namespace Pds {

  class MonMessage;

  class VmonSocket : public MonSocket {
  public:
    VmonSocket();
    ~VmonSocket();
    
    virtual int readv(const iovec* iov, int iovcnt);

    int peek (MonMessage*);
    int flush();

  protected:
    struct msghdr _rhdr;
  private:
    iovec*        _iovs;
    int           _iovcnt;
    bool          _peeked;
  };
};

#endif
