#include <stdio.h>
#include <string.h>

#include "Arp.hh"
#include "Node.hh"
#include "Route.hh"

using namespace Pds;

int Arp::added(int ipaddress)
{
  ipaddress &= 0x3ff;
  int word = ipaddress>>5;
  int bit = ipaddress&0x1f;
  if (_added[word] & (1<<bit)) {
    return 1;
  } else {
    _added[word] |= (1<<bit);
    return 0;
  }
}

int Arp::add(const Node& hdr) 
{
  if (added(hdr.ip())) return 0;
  int result = add(Route::name(), hdr.ip(), hdr.ether());
  if (result) {
    char str[20];
    printf("*** Arp::add set entry 0x%x -> %s error: %s\n",
	   hdr.ip(), hdr.ether().as_string(str), strerror(result));
//   } else {
//     char str[20];
//     printf("Arp::add set entry 0x%x -> %s\n",
// 	   hdr.ip(), hdr.ether().as_string(str));
  }
  return result;
}
