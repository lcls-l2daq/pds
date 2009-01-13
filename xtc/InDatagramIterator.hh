#ifndef Pds_InDatagramIterator_hh
#define Pds_InDatagramIterator_hh

#include <sys/socket.h>

namespace Pds {

  class InDatagramIterator {
  public:
    virtual ~InDatagramIterator() {}

    //  skip over "len" bytes
    //  returns number of bytes skipped
    virtual int skip(int len) = 0;

    //  read "len" bytes and assign "iov" to reference them
    //  returns number of bytes read
    virtual int read(iovec* iov, int maxiov, int len) = 0;

    //  read "len" bytes.  
    //  if they are not already contiguous, copy them into "buffer".
    //  returns location of contiguous data.
    virtual void* read_contiguous(int len, void* buffer) = 0;

    //  copy "len" bytes to "dst"
    //  returns number of bytes copied
    int copy( void* dst, int len );
  };

}

#endif
