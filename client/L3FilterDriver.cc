#include "pds/client/L3FilterDriver.hh"

#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/L1AcceptEnv.hh"
#include "pdsdata/psddl/l3t.ddl.h"

#include <exception>

//#define DBUG

namespace Pds {
  class EpicsFinder : public XtcIterator {
  public:
    EpicsFinder(InDatagram& dg) : _dg(dg) {}
  public:
    int process(Xtc* xtc)
    {
      if (xtc->src.level()==Level::Segment) {
        const DetInfo& info = static_cast<const DetInfo&>(xtc->src);
        if (info.detector()==DetInfo::EpicsArch)
          _dg.insert(*xtc,xtc->payload());
      }
      else if (xtc->contains.id()==TypeId::Id_Xtc) {
        iterate(xtc);
      }
      return 1;
    }
  private:
    InDatagram& _dg;
    Xtc*        _xtc;
  };
};

using namespace Pds;

L3FilterDriver::L3FilterDriver(L3FilterModule* m) :
  _m     (m),
  _lVeto (false)
{
}

L3FilterDriver::~L3FilterDriver()
{
  delete _m;
}

Transition* L3FilterDriver::transitions(Transition* tr) { return tr; }

InDatagram* L3FilterDriver::events     (InDatagram* dg)
{
  if (dg->datagram().seq.service() == TransitionId::Configure) {
    _m->pre_configure();
    _event = false;
    iterate(&dg->datagram().xtc);
    _m->post_configure();
    
    std::string sname(_m->name());
    std::string sconf(_m->configuration());
#ifdef DBUG
    printf("L3FilterDriver [%p] inserting configuration\n",this);
    printf("%s:\n",sname.c_str());
    printf("%s\n", sconf.c_str());
#endif
    char* buff = new char[sizeof(L3T::ConfigV1)+sname.size()+sconf.size()+8];
    L3T::ConfigV1& c = *new(buff)L3T::ConfigV1(sname.size()+1, sconf.size()+1,
                                               sname.c_str(), sconf.c_str());
    Xtc cxtc(TypeId(TypeId::Type(c.TypeId),c.Version),
             dg->datagram().xtc.src);
    cxtc.extent = sizeof(Xtc)+c._sizeof();
    dg->insert(cxtc,&c);
    delete[] buff;
  }
  else if (dg->datagram().seq.service() == TransitionId::L1Accept) {

#ifdef DBUG
    printf("L3FilterDriver [%p] l1accept %d.%09d\n",this,
	   dg->datagram().seq.clock().seconds(),
	   dg->datagram().seq.clock().nanoseconds());
#endif

    unsigned extent = dg->datagram().xtc.extent;
    try {
      _m->pre_event();
      _event = true;
      iterate(&dg->datagram().xtc);

      if (_m->complete()) {
	bool lAccept = _m->accept();
	if (_lVeto && !lAccept) {
	  dg->datagram().xtc.extent = sizeof(Xtc);
	  dg->datagram().xtc.damage = Damage(0);

	  // insert EPICS data!!
	  //   this data may be needed by later events
	  EpicsFinder finder(*dg);
	  finder.iterate();
	}
      
        const L1AcceptEnv& l1 = static_cast<const L1AcceptEnv&>(dg->datagram().env);
        dg->datagram().env = L1AcceptEnv(l1.clientGroupMask(), 
                                         lAccept ? L1AcceptEnv::Pass : L1AcceptEnv::Fail,
                                         _lVeto && !lAccept);
      
        L3T::DataV1 payload(lAccept);
        Xtc dxtc(TypeId(TypeId::Type(payload.TypeId),payload.Version),
                 dg->datagram().xtc.src);
        dxtc.extent = sizeof(Xtc)+payload._sizeof();
        dg->insert(dxtc,&payload);

#ifdef DBUG
	printf("L3FilterDriver::event accept %c payload %d\n",
	       lAccept ? 't':'f', dg->datagram().xtc.sizeofPayload());
#endif
      }
    } 
    catch (std::exception& e) {
      printf("L3FilterDriver caught exception %s\n",e.what());
      dg->datagram().xtc.extent = extent;
    }
    catch (...) {
      printf("L3FilterDriver caught unknown exception\n");
      dg->datagram().xtc.extent = extent;
    }
  }
  return dg;
}

int L3FilterDriver::process(Xtc* xtc) {
  if (xtc->contains.id() == TypeId::Id_Xtc)
    iterate(xtc);
  else if (xtc->damage.value()!=0)
    ;
  else if (_event) {
    switch(xtc->src.level()) {
    case Level::Source:
      _m-> event(static_cast<const Pds::DetInfo&>(xtc->src),
                 xtc->contains,
                 xtc->payload());
      break;
    case Level::Reporter:
      _m-> event(static_cast<const Pds::BldInfo&>(xtc->src),
                 xtc->contains,
                 xtc->payload());
      break;
    default:
      _m-> event(static_cast<const Pds::ProcInfo&>(xtc->src),
                 xtc->contains,
                 xtc->payload());
      break;
    }
  }
  else {
    switch(xtc->src.level()) {
    case Level::Source:
      _m-> configure(static_cast<const Pds::DetInfo&>(xtc->src),
                     xtc->contains,
                     xtc->payload());
      break;
    case Level::Reporter:
      _m-> configure(static_cast<const Pds::BldInfo&>(xtc->src),
                     xtc->contains,
                     xtc->payload());
      break;
    default:
      _m-> configure(static_cast<const Pds::ProcInfo&>(xtc->src),
                     xtc->contains,
                     xtc->payload());
      break;
    }
  }
  return 1;
}
