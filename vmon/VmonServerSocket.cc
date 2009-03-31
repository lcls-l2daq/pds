#include "pds/vmon/VmonServerSocket.hh"

#include <stdio.h>
#include <errno.h>
#include <string.h>

using namespace Pds;

VmonServerSocket::VmonServerSocket(const Ins& mcast,
				   int interface) :
  _peer     (Ins()),
  _mcast    (mcast),
  _interface(interface)
{
  int sockfd = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd<0) {
    printf("MonServerSocket failed to open socket\n");
    return;
  }

  int y=1;
  if((setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char*)&y, sizeof(y)) == -1) ||
     (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&y, sizeof(y)) == -1)) {
    printf("MSS error in setReuseAddr socket: %s\n", strerror(errno));
    return;
  }

  Sockaddr sa(mcast);
  int result = ::bind(sockfd, (sockaddr*)sa.name(), sa.sizeofName());
  if (result < 0) {
    printf("MSS error binding socket: %s\n", strerror(errno));
    return;
  }

  sockaddr_in name;
  socklen_t name_len=sizeof(name);
  getsockname(sockfd,(sockaddr*)&name,&name_len);
  printf("Socket %d bound to %x/%d\n",
	 sockfd, ntohl(name.sin_addr.s_addr), ntohs(name.sin_port));

  printf("joining %x on interface %x\n",mcast.address(),interface);

  struct ip_mreq ipMreq;
  bzero ((char*)&ipMreq, sizeof(ipMreq));
  ipMreq.imr_multiaddr.s_addr = htonl(mcast.address());
  ipMreq.imr_interface.s_addr = htonl(interface);
  int error_join = setsockopt (sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
			       (char*)&ipMreq, sizeof(ipMreq));
  if (error_join==-1) {
    printf("Error joining mcast group %x : reason %s\n", 
	   mcast.address(),strerror(errno));
    return;
  }

  _socket = sockfd;

  _hdr.msg_name    = &_peer;
  _hdr.msg_namelen = sizeof(_peer);
  _rhdr.msg_name    = &_peer;
  _rhdr.msg_namelen = sizeof(_peer);
}

VmonServerSocket::~VmonServerSocket()
{
  struct ip_mreq ipMreq;
  bzero ((char*)&ipMreq, sizeof(ipMreq));
  ipMreq.imr_multiaddr.s_addr = htonl(_mcast.address());
  ipMreq.imr_interface.s_addr = htonl(_interface);
  int error_resign = setsockopt (_socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&ipMreq,
				 sizeof(ipMreq));
  if (error_resign)
    printf("~MonServerSocket error resigning from group %x : reason %s\n",
	   _mcast.address(), strerror(errno));
}

