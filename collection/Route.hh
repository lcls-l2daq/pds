// Class Route provides the interface address and the broadcast
// address which should be used to send all PDS network data.
// Function set() compares the master crate address with the array of
// interfaces in RouteTable and the default PDS interface is set to
// be the one which belongs to the same network of the platform.


#ifndef PDSROUTE_HH
#define PDSROUTE_HH

#include "Ether.hh"
#include "RouteTable.hh"

namespace Pds {

class Route {
public:
  static void set(const RouteTable& table, int mastercrate);
  static const char* name();
  static int interface();
  static int broadcast();
  static int netmask();
  static const Ether& ether();
  static int mastercrate();

private:
  static char _name[RouteTable::MaxNameLen];
  static int _interface;
  static int _broadcast;
  static int _netmask;
  static Ether _ether;
  static int _mastercrate;
};

}
#endif
