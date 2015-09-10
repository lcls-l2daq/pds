#ifndef Pds_EbClockKey_hh
#define Pds_EbClockKey_hh

#include "EbEventKey.hh"

#include "pds/xtc/Datagram.hh"
#include "pds/service/Pool.hh"
#include "pdsdata/xtc/Sequence.hh"
#include "EbSequenceSrv.hh"
#include "BldSequenceSrv.hh"
#include "EvrServer.hh"

namespace Pds {
  class EbClockKey : public EbEventKey {
  public:
    EbClockKey(Datagram& s) : key(s) { s.seq = Sequence(); s.env = 0; }
    PoolDeclare;
  public:
    virtual bool precedes (const EbSequenceSrv& s) { return !(key.seq.clock() > s.sequence().clock()); } 
    virtual bool coincides(const EbSequenceSrv& s) { return key.seq.clock() == s.sequence().clock(); } 
    virtual void assign   (const EbSequenceSrv& s) { key.seq = s.sequence(); key.env = s.env(); }

    virtual bool precedes (const EvrServer& s) { return !(key.seq.clock() > s.sequence().clock()); } 
    virtual bool coincides(const EvrServer& s) { return key.seq.clock() == s.sequence().clock(); } 
    virtual void assign   (const EvrServer& s) { key.seq = s.sequence(); }
  public:
    const Sequence& sequence() const { return key.seq; }
    const Env&      env     () const { return key.env; }
    unsigned        value   () const { return key.seq.stamp().fiducials(); }
  private:
    Datagram& key;
  };
}
#endif
