#ifndef Pds_MonENTRYDESCScalar_HH
#define Pds_MonENTRYDESCScalar_HH

#include "pds/mon/MonDescEntry.hh"

#include <vector>
#include <string>

namespace Pds {

  class MonDescScalar : public MonDescEntry {
  public:
    MonDescScalar(const char* name);
    MonDescScalar(const char* name, const std::vector<std::string>& names);
  public:
    unsigned elements() const { return _elements; }
    const char* names() const { return _names; }
  private:
    unsigned _elements;
    enum {NamesSize=256};
    char _names[NamesSize];
  };
};

#endif
