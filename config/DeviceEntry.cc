#include "pds/config/DeviceEntry.hh"

#include <stdio.h>

#include <sstream>
using std::istringstream;
using std::ostringstream;
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#include <iomanip>
using std::setw;
using std::setfill;

using namespace Pds_ConfigDb;


DeviceEntry::DeviceEntry(unsigned id) :
  Pds::Src(Pds::Level::Source)
{
  _phy = id; 
}

DeviceEntry::DeviceEntry(uint64_t id) :
  Pds::Src(Pds::Level::Type(unsigned(id>>56))) 
{
  _phy = unsigned(id&0xffffffff);
}

DeviceEntry::DeviceEntry(const Pds::Src& id) :
  Pds::Src(id)
{
}

DeviceEntry::DeviceEntry(const string& id) 
{
  char sep;
  istringstream i(id);
  i >> std::hex >> _log >> sep >> _phy;
}

uint64_t DeviceEntry::value() const
{
  uint64_t v = _log;
  v = (v<<32) | _phy;
  return v;
}
