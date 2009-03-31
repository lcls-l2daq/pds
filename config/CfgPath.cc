#include "pds/config/CfgPath.hh"

#include "pdsdata/xtc/TypeId.hh"


using namespace Pds;


string CfgPath::path(unsigned key, const Src& src, const TypeId& type)
{
  ostringstream o;
  o << std::hex << setfill('0') << setw(8) << key << '/' << CfgPath::src_key(src) << '/' << setw(8) << type.value();
  return o.str();
}
