#ifndef Pds_PlatformCallback_hh
#define Pds_PlatformCallback_hh

namespace Pds {
  class Node;
  class PingReply;
  class AliasReply;
  class PlatformCallback {
  public:
    virtual ~PlatformCallback() {}
    virtual void available(const Node&, const PingReply&) = 0;
    virtual void aliasCollect(const Node&, const AliasReply&) = 0;
  };

};

#endif
