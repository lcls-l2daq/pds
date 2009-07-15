#ifndef Pds_PingReply_hh
#define Pds_PingReply_hh

#include "pds/collection/Message.hh"
#include "pdsdata/xtc/Src.hh"

#include <list>

namespace Pds {
  class PingReply : public Message {
  public:
    PingReply(const std::list<Src>& sources);
  public:
    unsigned   nsources() const;
    const Src& source(unsigned i) const;
  private:
    enum { MAX_SOURCES=4 };
    Src _sources[MAX_SOURCES];
  };
};

#endif
