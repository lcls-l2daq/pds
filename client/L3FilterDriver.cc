#include "pds/client/L3FilterDriver.hh"

#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/L1AcceptEnv.hh"
#include "pdsdata/psddl/l3t.ddl.h"

#include <exception>
#include <list>
#include <fstream>
#include <sstream>

#include <stdlib.h>

//#define DBUG

using namespace Pds;

L3FilterDriver::L3FilterDriver(L3FilterModule* m,
                               const char*     exp) :
  _m     (m),
  _lUse  (false),
  _lVeto (false),
  _iUnbias(0)
{
  if (exp)
    _m->set_experiment(std::string(exp));
}

L3FilterDriver::~L3FilterDriver()
{
  delete _m;
}

Transition* L3FilterDriver::transitions(Transition* tr) 
{
  if (tr->id()==TransitionId::Map) {
    const Allocation& alloc = static_cast<Allocate*>(tr)->allocation();
    _lUse    = alloc.l3tag () || alloc.l3veto();
    _lVeto   = alloc.l3veto();
    _path    = alloc.l3path();
    _iUnbias = int(alloc.unbiased_fraction()*RAND_MAX);
    if (_iUnbias<0) _iUnbias=RAND_MAX;
  }
  return tr; 
}

InDatagram* L3FilterDriver::events     (InDatagram* dg)
{
  if (!_lUse) return dg;

  if (dg->datagram().seq.service() == TransitionId::Configure) {

    std::ostringstream s;
    if (_path.size()) {
      std::ifstream i(_path.c_str());
      if (i.good()) {
	i >> s.rdbuf();
        printf("L3FilterDriver: Loaded configuration from %s [%zu bytes]\n",
               _path.c_str(),s.str().size());
      }
      else
	printf("L3FilterDriver: Error opening %s\n",_path.c_str());
    }
    _m->pre_configure(s.str());

    _event = false;
    iterate(&dg->datagram().xtc);

    Damage dmg(0);
    if (!_m->post_configure())
      dmg.increase(Damage::UserDefined);
    
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
             dg->datagram().xtc.src,
	     dmg);
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

    
    bool lUnbiased = (!_lVeto) || (rand()<_iUnbias);

    const L1AcceptEnv& l1 = static_cast<const L1AcceptEnv&>(dg->datagram().env);
    L1AcceptEnv l1e(l1.clientGroupMask(), 
		    L1AcceptEnv::None,
		    false, 
		    lUnbiased);

    L3T::DataV2 l3t(L3T::DataV2::None,
		    lUnbiased ? 
		    L3T::DataV2::Unbiased :
		    L3T::DataV2::Biased);

    unsigned extent = dg->datagram().xtc.extent;
    try {
      _m->pre_event();
      _event = true;
      iterate(&dg->datagram().xtc);

      if (_m->complete()) {
	bool lAccept = _m->accept();
	bool lTrim = _lVeto && !lAccept && !lUnbiased;
	if (lTrim) {
          dg->datagram().xtc.extent = sizeof(Xtc);
          dg->datagram().xtc.damage = Damage(0);
	}
      
	l3t = L3T::DataV2(lAccept ? 
			  L3T::DataV2::Pass :
			  L3T::DataV2::Fail,
			  l3t.bias());

	l1e = L1AcceptEnv(l1.clientGroupMask(), 
			  lAccept ? L1AcceptEnv::Pass : L1AcceptEnv::Fail,
			  lTrim, 
			  lUnbiased);
      
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

    dg->datagram().env = l1e;
    Xtc dxtc(TypeId(TypeId::Type(l3t.TypeId),l3t.Version),
	     dg->datagram().xtc.src);
    dxtc.extent = sizeof(Xtc)+l3t._sizeof();
    dg->insert(dxtc,&l3t);

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
