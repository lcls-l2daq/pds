#ifndef Pds_EbSequenceKey_hh
#define Pds_EbSequenceKey_hh

#include "EbEventKey.hh"

#include "pds/xtc/Datagram.hh"
#include "pds/service/Pool.hh"
#include "EbSequenceSrv.hh"

namespace Pds {
  class EbSequenceKey : public EbEventKey {
  public:
    EbSequenceKey(Datagram& s) : key(s) { s.seq = Sequence(); s.env = 0; }
    PoolDeclare;
  public:
    virtual bool precedes (const EbSequenceSrv& s) { return key.seq <= s.sequence(); } 
    virtual bool coincides(const EbSequenceSrv& s) { return key.seq == s.sequence(); } 
    virtual void assign   (const EbSequenceSrv& s) { key.seq = s.sequence(); key.env = s.env(); }
  public:
    const Sequence& sequence() const { return key.seq; }
    const Env&      env     () const { return key.env; }
  private:
    Datagram& key;
  };
}
#endif
