#ifndef Pds_EbSequenceKey_hh
#define Pds_EbSequenceKey_hh

#include "EbEventKey.hh"

#include "pds/xtc/Datagram.hh"
#include "pds/service/Pool.hh"
#include "pdsdata/xtc/Sequence.hh"
#include "EbSequenceSrv.hh"
#include "BldSequenceSrv.hh"
#include "EvrServer.hh"

namespace Pds {
  class EbSequenceKey : public EbEventKey {
  public:
    EbSequenceKey(Datagram& s) : key(s) { s.seq = Sequence(); s.env = 0; }
    PoolDeclare;
  public:
    virtual bool precedes (const EbSequenceSrv& s) { return key.seq.stamp() <= s.sequence().stamp(); } 
    virtual bool coincides(const EbSequenceSrv& s) { return key.seq.stamp() == s.sequence().stamp(); } 
    virtual void assign   (const EbSequenceSrv& s) { key.seq = s.sequence(); key.env = s.env(); }

    virtual bool precedes (const BldSequenceSrv& s) { return key.seq.stamp().fiducials() <= s.fiducials(); } 
    virtual bool coincides(const BldSequenceSrv& s) { return key.seq.stamp().fiducials() == s.fiducials(); } 
    virtual void assign   (const BldSequenceSrv& s) 
    {
      const TimeStamp& ts = key.seq.stamp();
      key.seq = Sequence(key.seq.clock(),
			 TimeStamp(ts.ticks(),
				   s.fiducials(),
				   ts.vector(),
				   ts.control()));
    }

    virtual bool precedes (const EvrServer& s) { return key.seq.stamp() <= s.sequence().stamp(); } 
    virtual bool coincides(const EvrServer& s) { return key.seq.stamp() == s.sequence().stamp(); } 
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
