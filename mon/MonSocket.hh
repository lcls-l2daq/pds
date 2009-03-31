#ifndef Pds_MonSOCKET_HH
#define Pds_MonSOCKET_HH

#include <sys/types.h>
#include <sys/socket.h>

class iovec;

namespace Pds {

  class Ins;

  class MonSocket {
  public:
    MonSocket(int socket=-1);
    virtual ~MonSocket();
    
    int socket() const;
    
    virtual int readv(const iovec* iov, int iovcnt) = 0;

    int read(void* buffer, int size);

    int write (const void* data, int size);
    int writev(const iovec* iov, int iovcnt);
    
    int setsndbuf(unsigned size);
    int setrcvbuf(unsigned size);
    
    int close();
    
  protected:
    int _socket;
    struct msghdr _hdr;
  };
};

#endif
