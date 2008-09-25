#ifndef Pds_EbEventKey_hh
#define Pds_EbEventKey_hh

namespace Pds {

  class Sequence;
  class BldServer;
  class EvrServer;
  class NetDgServer;
  class ToEb;

#define EbServerDeclare \
    bool        succeeds (EbEventKey& key) const { return key.precedes(*this); } \
    bool        coincides(EbEventKey& key) const { return key.coincides(*this); } \
    void        assign   (EbEventKey& key) const { return key.assign(*this); } \


#define EbEventKeyDeclare(server) \
    virtual bool precedes (const server &) { return false; } \
    virtual bool coincides(const server &) { return false; } \
    virtual void assign   (const server &) {} \

  class EbEventKey {
  public:
    virtual ~EbEventKey() {}
  public:
    virtual const Sequence& sequence() const = 0;
  public:
    EbEventKeyDeclare(BldServer);
    EbEventKeyDeclare(EvrServer);
    EbEventKeyDeclare(NetDgServer);
    EbEventKeyDeclare(ToEb);
  };

}

#endif
