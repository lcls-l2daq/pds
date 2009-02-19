#include <sys/types.h> // Added to compile on Mac, shouldn't be needed
#include <sys/socket.h>

#include "pds/mon/MonSocketAddress.hh"
#include "pds/service/Ins.hh"

using namespace Pds;
 
MonSocketAddress::MonSocketAddress(const Ins& ip) 
{
  _sockaddr.sin_family      = AF_INET;
  _sockaddr.sin_addr.s_addr = htonl(ip.address());
  _sockaddr.sin_port        = htons(ip.portId()); 
}

sockaddr* MonSocketAddress::name() const {return (sockaddr*)&_sockaddr;}
int MonSocketAddress::sizeofName() const {return sizeof(_sockaddr);}
