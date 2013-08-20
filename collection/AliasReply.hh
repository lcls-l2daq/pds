#ifndef Pds_AliasReply_hh
#define Pds_AliasReply_hh

#include "pds/collection/Message.hh"
#include "pdsdata/xtc/SrcAlias.hh"

#include <list>

namespace Pds {
  class AliasReply : public Message {
  public:
    AliasReply();
    AliasReply(const std::list<SrcAlias>& aliases);
  public:
    unsigned   naliases() const;
    const SrcAlias& alias(unsigned i) const;
  private:
    enum { MAX_ALIASES=64 };
    SrcAlias _aliases[MAX_ALIASES];
  };
};

#endif
