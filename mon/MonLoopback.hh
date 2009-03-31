#ifndef Pds_MonLoopback_HH
#define Pds_MonLoopback_HH

#include "pds/mon/MonSocket.hh"

namespace Pds {

  class Ins;

  class MonLoopback : public MonSocket {
  public:
    MonLoopback();
    ~MonLoopback();
    virtual int readv(const iovec* iov, int iovcnt);
  };
};

#endif
