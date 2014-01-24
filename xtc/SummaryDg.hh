#ifndef Pds_SummaryDg_hh
#define Pds_SummaryDg_hh

#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/XtcType.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/xtc/L1AcceptEnv.hh"

//#define DBUG

static Pds::BldInfo EBeamBPM(uint32_t(-1UL),Pds::BldInfo::EBeam);

namespace Pds {
  namespace SummaryDg {
    class Xtc : public Pds::Xtc {
    public:
      static TypeId typeId() { return _l3SummaryType; }
    public:
      Xtc() : Pds::Xtc() {}
      ~Xtc() {}
    public:
      unsigned payload() const;
      L1AcceptEnv::L3TResult l3tresult() const;
      unsigned nSources() const;
      const Pds::Src& source(unsigned i) const;
    };

    class Dg : public Pds::CDatagram {
    public:
      Dg(const Pds::Datagram& dg);
      ~Dg() {}
    public:
      void append(const Pds::Src& info, const Pds::Damage& dmg);
      void append(L1AcceptEnv::L3TResult v);
    private:
      static Dgram _dgram(const Pds::Datagram& dg);
    private:
      uint32_t _payload;
      Src      _info[32];
    };
  };
};


inline unsigned Pds::SummaryDg::Xtc::payload() const 
{ return *reinterpret_cast<const uint32_t*>(Pds::Xtc::payload())&0x3fffffff; }

inline Pds::L1AcceptEnv::L3TResult Pds::SummaryDg::Xtc::l3tresult() const
{ return L1AcceptEnv::L3TResult(*reinterpret_cast<const uint32_t*>(Pds::Xtc::payload())>>30); }

inline unsigned Pds::SummaryDg::Xtc::nSources() const
{ return (sizeofPayload()-sizeof(uint32_t))/sizeof(Pds::Src); }

inline const Pds::Src& Pds::SummaryDg::Xtc::source(unsigned i) const
{
  uint32_t* p = reinterpret_cast<uint32_t*>(Pds::Xtc::payload());
  return reinterpret_cast<const Pds::Src*>(p+1)[i];
}

#define ExtendedBit (1<<7)

inline Pds::Dgram Pds::SummaryDg::Dg::_dgram(const Datagram& dg)
{
  Dgram d;
  d.seq = Pds::Sequence(dg.seq.clock(),
                        Pds::TimeStamp(dg.seq.stamp(),
                                       dg.seq.stamp().control()&~ExtendedBit));
  d.env = dg.env;
  d.xtc = Pds::Xtc(Pds::SummaryDg::Xtc::typeId(),dg.xtc.src);
  return d;
}

#undef ExtendedBit

inline Pds::SummaryDg::Dg::Dg(const Datagram& dg) : 
  CDatagram(_dgram(dg)),
  _payload(dg.xtc.sizeofPayload())
{
  datagram().xtc.alloc(sizeof(_payload));
}

inline void Pds::SummaryDg::Dg::append(const Pds::Src& info, 
                                       const Pds::Damage& dmg) 
{
#ifdef DBUG
  printf("SummaryDg::append %08x.%08x %08x\n",
         info.log(),info.phy(),dmg.value());
#endif
  datagram().xtc.damage.increase(dmg.value());
  *static_cast<Pds::Src*>(datagram().xtc.alloc(sizeof(info))) = info;
}

inline void Pds::SummaryDg::Dg::append(Pds::L1AcceptEnv::L3TResult v)
{ _payload |= (unsigned(v)<<30); }

#endif
