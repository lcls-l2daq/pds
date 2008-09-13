#ifndef Pds_InDatagramIterator_hh
#define Pds_InDatagramIterator_hh

#include <sys/socket.h>

namespace Pds {

  class InDatagramIterator {
  public:
    virtual ~InDatagramIterator() {}

    virtual int skip(int len) = 0;
    virtual int read(iovec*, int maxiov, int len) = 0;
    virtual int copy(void*, int len) = 0;
  };

}

#endif
