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
    //EbCountKey(Sequence& s) : key((unsigned)-1), seq(s) 
    //{
    //  //  Initialize the sequence to a default
    //  s = Sequence(Sequence::Event, 
    //   TransitionId::L1Accept,
    //   ClockTime(),
    //   TimeStamp()); 
    //}
    
    EbCountKey(Datagram& dg) : key((unsigned)-1), dgram(dg) { 
      //  Initialize the sequence to a default
      dg.seq = Sequence(); 
      dg.env = 0;
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
    virtual void assign   (const EvrServer& s) { key = s.count(); dgram.seq = s.sequence(); dgram.env = s.env();}
    //  Special case for only one server providing timestamp
    virtual bool precedes (const EbSequenceSrv& s) { return false; }
    virtual bool coincides(const EbSequenceSrv& s) { return false; }
    virtual void assign   (const EbSequenceSrv& s) { dgram.seq = s.sequence(); dgram.env = s.env(); }
  public:
    const Sequence& sequence() const { return dgram.seq; }
  private:  
    unsigned  key;
    //Sequence& seq;
    Datagram& dgram;
  };
}
#endif
