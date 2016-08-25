#ifndef TagServer_hh
#define TagServer_hh

#include "pds/utility/NullServer.hh"

namespace Pds {
  class TagServer : public NullServer {
  public:
    TagServer(const Ins& ins,
              const Src& src,
              unsigned   maxsiz,
              unsigned   maxevt) : NullServer(ins,src,maxsiz,maxevt) {}
  public:
    bool isValued  () const { return true; }
    bool isRequired() const { return true; }
  };
};

#endif
