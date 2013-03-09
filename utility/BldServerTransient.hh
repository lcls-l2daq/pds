#ifndef Pds_BldServerTransient_hh
#define Pds_BldServerTransient_hh

#include "pds/utility/BldServer.hh"

namespace Pds {

  class BldServerTransient : public BldServer {
  public:
    BldServerTransient(const Ins& ins,
		       const Src& src,
		       unsigned   maxbuf);
   ~BldServerTransient();
  public:
    const Xtc&     xtc   () const;
    unsigned       length() const;
    unsigned       offset() const;
  public:
    //  Server interface
    int      fetch       (char* payload, int flags);
  private:
    Xtc* _xtc;
  };
};

#endif
