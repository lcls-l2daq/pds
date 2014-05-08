// The class RouteTable builds an array of the interfaces which are
// found in the machine.  The implementation is platform dependent and
// so there are two implementation files: RouteTableUx.cc and
// RouteTableVx.cc.

#ifndef PDSROUTETABLE_HH
#define PDSROUTETABLE_HH

#include "Ether.hh"

namespace Pds {

class RouteTable {
public:
  RouteTable();

  static const unsigned MaxRoutes = 10;
  static const unsigned MaxNameLen = 16;

  unsigned routes() const;
  bool     routes(unsigned index, int dst) const;
  const char* name(unsigned index) const;
  int interface(unsigned index) const;
  int broadcast(unsigned index) const;
  int netmask(unsigned index) const;
  const Ether& ether(unsigned index) const;

private:
  unsigned _found;
  char _name[MaxRoutes*MaxNameLen];
  int _iface[MaxRoutes];
  int _dst  [MaxRoutes];
  int _netma[MaxRoutes];
  int _bcast[MaxRoutes];
  Ether _ether[MaxRoutes];
};

}
#endif
