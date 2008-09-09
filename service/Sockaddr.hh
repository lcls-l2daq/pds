#ifndef PDS_SOCKADDR
#define PDS_SOCKADDR

namespace Pds {
class Sockaddr {
public:
  Sockaddr() {
    _sockaddr.sin_family      = AF_INET;
  }
  
  Sockaddr(const Ins& ins) {
    _sockaddr.sin_family      = AF_INET;
    _sockaddr.sin_addr.s_addr = htonl(ins.address());
    _sockaddr.sin_port        = htons(ins.portId()); 
  }
  
  void get(const Ins& ins) {
    _sockaddr.sin_addr.s_addr = htonl(ins.address());
    _sockaddr.sin_port        = htons(ins.portId());     
  }

  sockaddr* name() const {return (sockaddr*)&_sockaddr;}
  inline int sizeofName() const {return sizeof(_sockaddr);}

private:
  sockaddr_in _sockaddr;
};
}
#endif
