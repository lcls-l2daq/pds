#include <string.h>

#include "Route.hh"

using namespace Pds;

char Route::_name[RouteTable::MaxNameLen];
int Route::_interface = 0;
int Route::_broadcast = 0;
int Route::_netmask = 0;
Ether Route::_ether;
int Route::_mastercrate = 0;

void Route::set(const RouteTable& table, int mastercrate)
{
  _mastercrate = mastercrate;
  for (unsigned index=0; index<table.routes(); index++) {
    if (table.routes(index,mastercrate)) {
      strncpy(_name, table.name(index), RouteTable::MaxNameLen-1);
      _interface = table.interface(index);
      _broadcast = table.broadcast(index);
      _netmask   = table.netmask(index);
      _ether     = table.ether(index);
      break;
    }
  }
}

const char* Route::name() {return _name;}
int Route::interface() {return _interface;}
int Route::broadcast() {return _broadcast;}
int Route::netmask() {return _netmask;}
const Ether& Route::ether() {return _ether;}
int Route::mastercrate() {return _mastercrate;}
