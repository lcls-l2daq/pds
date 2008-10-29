#ifndef Pds_EbSequenceKey_hh
#define Pds_EbSequenceKey_hh

#include "EbEventKey.hh"

#include "pds/service/Pool.hh"
#include "EbSequenceSrv.hh"

#define EbSequenceKeyDeclare( server ) \

namespace Pds {
  class EbSequenceKey : public EbEventKey {
  public:
    EbSequenceKey(Sequence& s) : key(s) { s = Sequence(); }
    PoolDeclare;
  public:
    virtual bool precedes (const EbSequenceSrv& s) { return key <= s.sequence(); } \
    virtual bool coincides(const EbSequenceSrv& s) { return key == s.sequence(); } \
    virtual void assign   (const EbSequenceSrv& s) { key = s.sequence(); }
  public:
    const Sequence& sequence() const { return key; }
  private:
    Sequence& key;
  };
}
#endif
