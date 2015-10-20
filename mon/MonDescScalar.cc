#include "pds/mon/MonDescScalar.hh"

using namespace Pds;

MonDescScalar::MonDescScalar(const char* name) : 
  MonDescEntry(name, "", "",
	       MonDescEntry::Scalar, 
	       sizeof(MonDescScalar)),
  _elements(1)
{
  _names[0] = 0;
}

MonDescScalar::MonDescScalar(const char* name, const std::vector<std::string>& names) :
  MonDescEntry(name, "", "",
	       MonDescEntry::Scalar, 
	       sizeof(MonDescScalar)),
  _elements(names.size())
{
  char* dst = _names;
  const char* last = dst+NamesSize-1;
  for(std::vector<std::string>::const_iterator it=names.begin(); it!=names.end(); it++) {
    if (dst + it->size() + 1 < last) {
      sprintf(dst,"%s:",it->c_str());
      dst += it->size()+1;
    }
    else
      break;
  }
  if (dst > _names) 
    *--dst = 0;
}
