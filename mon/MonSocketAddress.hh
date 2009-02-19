#ifndef Pds_MonSOCKETADDRESS_HH
#define Pds_MonSOCKETADDRESS_HH

#include <sys/types.h> // Added to compile on Mac, shouldn't be needed
#include <sys/socket.h>// Added to compile on Solaris
#include <netinet/in.h>

namespace Pds {

  class Ins;

  class MonSocketAddress {
  public:
    MonSocketAddress(const Ins& ip);
    
    sockaddr* name() const;
    int sizeofName() const;
    
  private:
    sockaddr_in _sockaddr;
  };
};
#endif
