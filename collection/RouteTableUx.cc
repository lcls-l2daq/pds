#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>

#ifdef __linux__
#include <sys/ioctl.h>
#else
#include <sys/sockio.h>
#include <net/if_arp.h>
#endif

#include "RouteTable.hh"

using namespace Pds;


RouteTable::RouteTable() :
  _found(0)
{
  int fd;  
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) return;

  struct ifreq ifrarray[MaxRoutes];

  memset(ifrarray, 0, sizeof(ifreq)*MaxRoutes);
  memset(_name,    0, sizeof(char)*MaxRoutes*MaxNameLen);
  memset(_iface,   0, sizeof(int)*MaxRoutes);
  memset(_netma,   0, sizeof(int)*MaxRoutes);
  memset(_bcast,   0, sizeof(int)*MaxRoutes);

  struct ifconf ifc;
  ifc.ifc_len = MaxRoutes * sizeof(struct ifreq);
  ifc.ifc_buf = (char*)ifrarray;

  if (ioctl(fd, SIOCGIFCONF, &ifc) < 0) {
    printf("*** RouteTable unable to config interfaces: too many? "
	   "(current limit is %d)\n", MaxRoutes);
    close(fd);
    return;
  }

  for (unsigned i=0; i<MaxRoutes; i++) {
    struct ifreq* ifr = ifrarray+i;
    if (!ifr || !(((sockaddr_in&)ifr->ifr_addr).sin_addr.s_addr)) break;

    strncpy(_name+_found*MaxNameLen, ifr->ifr_name, MaxNameLen-1);

    struct ifreq ifreq_netmask = *ifr;
    if (ioctl(fd, SIOCGIFNETMASK, &ifreq_netmask) < 0) {
      printf("*** RouteTable error getting netmask for '%s': %s\n",
	     name(_found), strerror(errno));
    }
    
    struct ifreq ifreq_flags = *ifr;
    if (ioctl(fd, SIOCGIFFLAGS, &ifreq_flags) < 0) {
      printf("*** RouteTable error getting flags for '%s': %s\n",
	     name(_found), strerror(errno));
    }
    int flags = ifreq_flags.ifr_flags;

    if ((flags & IFF_UP) && (flags & IFF_BROADCAST)) {

#ifdef __linux__
      struct ifreq ifreq_hwaddr = *ifr;
      if (ioctl(fd, SIOCGIFHWADDR, &ifreq_hwaddr) < 0)  {
#else
      struct arpreq ifreq_hwaddr;
      ifreq_hwaddr.arp_pa = (sockaddr&)ifr->ifr_addr;
      if (ioctl(fd, SIOCGARP, &ifreq_hwaddr) < 0)  {
#endif
	printf("*** RouteTable error getting hwaddr for '%s': %s\n",
	       name(_found), strerror(errno));
      }

      _iface[_found] = ntohl((((sockaddr_in&)ifr->ifr_addr).sin_addr.s_addr));
      _netma[_found] = ntohl(((sockaddr_in&)ifreq_netmask.ifr_addr).sin_addr.s_addr);
      _bcast[_found] = _iface[_found] | ~_netma[_found];
#ifdef __linux__
      _ether[_found] = Ether((unsigned char*)ifreq_hwaddr.ifr_hwaddr.sa_data);
#else
      _ether[_found] = Ether((unsigned char*)ifreq_hwaddr.arp_ha.sa_data);
#endif
      ++_found;
    }
  }
  close(fd);
}

