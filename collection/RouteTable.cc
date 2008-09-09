#include "RouteTable.hh"

using namespace Pds;

unsigned RouteTable::routes() const {return _found;}
const char* RouteTable::name(unsigned index) const 
{return _name+index*MaxNameLen;}
int RouteTable::interface(unsigned index) const {return _iface[index];}
int RouteTable::broadcast(unsigned index) const {return _bcast[index];}
int RouteTable::netmask(unsigned index) const {return _netma[index];}
const Ether& RouteTable::ether(unsigned index) const 
{return _ether[index];}
