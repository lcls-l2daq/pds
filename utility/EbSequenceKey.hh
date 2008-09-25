#ifndef Pds_EbSequenceKey_hh
#define Pds_EbSequenceKey_hh

#include "EbEventKey.hh"

#include "pds/service/Pool.hh"
#include "BldServer.hh"
#include "EvrServer.hh"
#include "NetDgServer.hh"
#include "ToEb.hh"

#define EbSequenceKeyDeclare( server ) \
    virtual bool precedes (const server& s) { return key <= s.sequence(); } \
    virtual bool coincides(const server& s) { return key == s.sequence(); } \
    virtual void assign   (const server& s) { key = s.sequence(); }
//#define EbSequenceKeyDeclare( server ) \
    virtual bool precedes (const server& s) { return key <= s.sequence(); } \
    virtual bool coincides(const server& s) { return key == s.sequence(); } \
    virtual void assign   (const server& s) \
    { printf("%s assign %08x/%08x\n",#server, \
      s.sequence().high(), s.sequence().low()); \
      key = s.sequence(); }

namespace Pds {
  class EbSequenceKey : public EbEventKey {
  public:
    EbSequenceKey(Sequence& s) : key(s) { s = Sequence(); }
  public:
    PoolDeclare;
    EbSequenceKeyDeclare(BldServer);
    EbSequenceKeyDeclare(EvrServer);
    EbSequenceKeyDeclare(NetDgServer);
    EbSequenceKeyDeclare(ToEb);
  public:
    const Sequence& sequence() const { return key; }
  private:
    Sequence& key;
  };
}
#endif
