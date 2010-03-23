#ifndef Pds_EbCountKey_hh
#define Pds_EbCountKey_hh

#include "EbEventKey.hh"

#include "EvrServer.hh"
#include "EbCountSrv.hh"
#include "EbSequenceSrv.hh"

#include "pds/service/Pool.hh"

namespace Pds {
  class EbCountKey : public EbEventKey {
  public:
    EbCountKey(Sequence& s) : key(-1UL), seq(s) 
    {
      //  Initialize the sequence to a default
      s = Sequence(Sequence::Event, 
		   TransitionId::L1Accept,
		   ClockTime(),
		   TimeStamp()); 
    }
  public:
    PoolDeclare;
  public:
    virtual bool precedes (const EbCountSrv& s) { return key <= s.count(); }
    virtual bool coincides(const EbCountSrv& s) { return key == s.count(); }
    virtual void assign   (const EbCountSrv& s) { key = s.count(); }
    //  Special service from the EVR
    virtual bool precedes (const EvrServer& s) { return key <= s.count(); }
    virtual bool coincides(const EvrServer& s) { return key == s.count(); }
    virtual void assign   (const EvrServer& s) { key = s.count(); seq = s.sequence(); }
    //  Special case for only one server providing timestamp
    virtual bool precedes (const EbSequenceSrv& s) { return false; }
    virtual bool coincides(const EbSequenceSrv& s) { return false; }
    virtual void assign   (const EbSequenceSrv& s) { seq = s.sequence(); }
  public:
    const Sequence& sequence() const { return seq; }
  public:
    unsigned  key;
    Sequence& seq;
  };
}
#endif
