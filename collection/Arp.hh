#ifndef PDSARP_HH
#define PDSARP_HH

namespace Pds {

class Ether;
class Node;

class Arp {
public:
#ifndef VXWORKS
  Arp(const char* suidprocess);
  int error() const;
#else
  Arp();
#endif
  ~Arp();

  int add(const char* dev, int ipaddress, const Ether& ether);
  int add(const Node& hdr);

private:
  int added(int ipaddress);
  unsigned _added[32];

private:
#ifndef VXWORKS
  int _pid;
  int _fd1[2];
  int _fd2[2];
  int _error;
#endif
};

}
#endif
