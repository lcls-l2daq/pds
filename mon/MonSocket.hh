#ifndef Pds_MonSOCKET_HH
#define Pds_MonSOCKET_HH

class iovec;

namespace Pds {

  class Ins;

  class MonSocket {
  public:
    MonSocket();
    MonSocket(int socket);
    virtual ~MonSocket();
    
    int socket() const;
    
    int read(void* buffer, int size) const;
    int write(const void* data, int size) const;
    
    int readv(const iovec* iov, int iovcnt) const;
    int writev(const iovec* iov, int iovcnt) const;
    
    int setsndbuf(unsigned size);
    int setrcvbuf(unsigned size);
    
    int connect(const Ins& dst);
    int listen (const Ins& src);
    int accept();
    int loopback();
    
    int close();
    
  private:
    int _socket;
  };
};

#endif
