#include "pds/mon/MonDescScalar.hh"
#include <string.h>

#include <stdio.h>

using namespace Pds;

MonDescScalar::MonDescScalar(const char* name) : 
  MonDescEntry(name, "", "",
	       MonDescEntry::Scalar, 
	       sizeof(MonDescScalar)),
  _elements(1)
{
  _names[0] = 0;
}

MonDescScalar::MonDescScalar(const char* name, const std::vector<std::string>& n) :
  MonDescEntry(name, "", "",
	       MonDescEntry::Scalar, 
	       sizeof(MonDescScalar)),
  _elements(n.size())
{
  set_names(n);
}

std::vector<std::string> MonDescScalar::get_names() const
{
  std::vector<std::string> result(elements());
  const char* src = _names;
  for(unsigned i=0; i<elements(); i++) {
    const char* next = strchr(src,':');
    if (next) {
      result[i] = std::string(src, next-src);
      src=next+1;
    }
    else {
      result[i] = std::string(src);
      src=src+strlen(src);
    }
  }
  return result;
}

void MonDescScalar::set_names(const std::vector<std::string>& src)
{
  char* dst = _names;
  const char* last = dst+NamesSize-1;
  for(std::vector<std::string>::const_iterator it=src.begin(); it!=src.end(); it++) {
    if (dst + it->size() + 1 < last) {
      sprintf(dst,"%s:",it->c_str());
      dst += it->size()+1;
    }
    else
      break;
  }
  if (dst > _names) 
    *--dst = 0;
  else
    _names[0] = 0;
}
