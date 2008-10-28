#ifndef Pds_EbCountKey_hh
#define Pds_EbCountKey_hh

#include "EbEventKey.hh"

#include "EvrServer.hh"
#include "ToEb.hh"
#include "AcqServer.hh"
#include "pds/service/Pool.hh"

#define EbCountKeyDeclare( server ) \
    virtual bool precedes (const server& s) { return key <= s.count(); } \
    virtual bool coincides(const server& s) { return key == s.count(); }

namespace Pds {
  class EbCountKey : public EbEventKey {
  public:
    EbCountKey(Sequence& s) : key(-1UL), seq(s) {}
  public:
    PoolDeclare;
    EbCountKeyDeclare(EvrServer);
    virtual void assign   (const EvrServer& s) { 
      key = s.count(); seq = s.sequence(); }
    // L1 Servers here
    EbCountKeyDeclare(ToEb);
    virtual void assign   (const ToEb& s) { 
      key = s.count(); }
    EbCountKeyDeclare(AcqServer);
    virtual void assign   (const AcqServer& s) { 
      //      printf("%s assign %08x\n","ToEb",
      //	     s.count());
      key = s.count(); }
  public:
    const Sequence& sequence() const { return seq; }
  public:
    unsigned  key;
    Sequence& seq;
  };
}
#endif
