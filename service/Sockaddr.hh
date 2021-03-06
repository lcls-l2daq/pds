#ifndef PDS_SOCKADDR
#define PDS_SOCKADDR

#include "pds/service/Ins.hh"

#include <sys/socket.h>
#include <string.h>

namespace Pds {
class Sockaddr {
public:
  Sockaddr() {
    memset(&_sockaddr,0,sizeof(_sockaddr));
    _sockaddr.sin_family      = AF_INET;
  }
  
  Sockaddr(const Ins& ins) {
    memset(&_sockaddr,0,sizeof(_sockaddr));
    _sockaddr.sin_family      = AF_INET;
    _sockaddr.sin_addr.s_addr = htonl(ins.address());
    _sockaddr.sin_port        = htons(ins.portId()); 
  }
  
  void get(const Ins& ins) {
    _sockaddr.sin_addr.s_addr = htonl(ins.address());
    _sockaddr.sin_port        = htons(ins.portId());     
  }

  Ins get() const {
    return Ins(ntohl(_sockaddr.sin_addr.s_addr),
	       ntohs(_sockaddr.sin_port));
  }

  sockaddr* name() const {return (sockaddr*)&_sockaddr;}
  inline int sizeofName() const {return sizeof(_sockaddr);}

private:
  sockaddr_in _sockaddr;
};
}
#endif
