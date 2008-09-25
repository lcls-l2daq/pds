#ifndef BldDatagram_hh
#define BldDatagram_hh

#include "Sequence.hh"

#include <string.h>

namespace Pds {

  class BldDatagram {
  public:
    BldDatagram(const Sequence& t, unsigned sz) : _sequence(t), _extent(sz) {}

    Sequence & sequence()       { return _sequence; }
    char*      payload ()       { return reinterpret_cast<char*>(this+1); }
    unsigned   sizeOfPayload() const { return _extent; }
  private:
    Sequence _sequence;
    unsigned _extent;
  };

}
#endif
