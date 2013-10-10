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
      uint32_t _payload;
      char     _info[32*sizeof(Pds::Src)];
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

inline Pds::SummaryDg::Dg::Dg(const Datagram& dg) : 
  CDatagram(dg,Pds::SummaryDg::Xtc::typeId(),dg.xtc.src),
  _payload(dg.xtc.sizeofPayload())
{
  datagram().xtc.damage.increase(dg.xtc.damage.value());
  datagram().xtc.alloc(sizeof(_payload));
  datagram().xtc.damage.increase(dg.xtc.damage.value());
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
