#include "pds/client/L3FilterDriver.hh"

#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/psddl/l3t.ddl.h"

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
    char* buff = new char[sizeof(L3T::ConfigV1)+sname.size()+sconf.size()+4];
    L3T::ConfigV1& c = *new(buff)L3T::ConfigV1(sname.size() , sconf.size(),
                                               sname.c_str(), sconf.c_str());
    Xtc cxtc(TypeId(TypeId::Type(c.TypeId),c.Version),
             dg->datagram().xtc.src);
    cxtc.extent = sizeof(Xtc)+c._sizeof();
    dg->insert(cxtc,&c);
    delete[] buff;
  }
  else if (dg->datagram().seq.service() == TransitionId::L1Accept) {
    _event = true;
    iterate(&dg->datagram().xtc);

    bool lAccept = _m->accept();
    if (_lVeto && !lAccept) {
      dg->datagram().xtc.extent = sizeof(Xtc);
      dg->datagram().xtc.damage = Damage(0);
    }

    L3T::DataV1 payload(lAccept);
    Xtc dxtc(TypeId(TypeId::Type(payload.TypeId),payload.Version),
             dg->datagram().xtc.src);
    dxtc.extent = sizeof(Xtc)+payload._sizeof();
    dg->insert(dxtc,&payload);
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
