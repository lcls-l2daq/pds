#ifndef Pds_L3TIterator_hh
#define Pds_L3TIterator_hh

#include "pdsdata/xtc/XtcIterator.hh"

namespace Pds {
  class L3TIterator : public XtcIterator {
  public:
    L3TIterator() : _found(false), _pass(false) {}
  public:
    bool found() const { return _found; }
    bool pass () const { return _pass; }
  public:
    int process(Xtc* xtc);
  private:
    bool _found;
    bool _pass;
  };
};

#endif
