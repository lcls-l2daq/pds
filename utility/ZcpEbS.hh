#ifndef PDS_ZCPEBS_HH
#define PDS_ZCPEBS_HH

#include "ZcpEb.hh"

#include "pds/service/GenericPool.hh"

namespace Pds {

class ZcpEbS : public ZcpEb
  {
  public:
    ZcpEbS(const Src& id,
	   const TypeId& ctns,
	   Level::Type level,
	   Inlet& inlet,
	   OutletWire& outlet,
	   int stream,
	   int ipaddress,
	   unsigned eventsize,
	   unsigned eventpooldepth,
	   VmonEb* vmoneb=0);
    ~ZcpEbS();

    void no_build(Sequence::Type type, unsigned mask);
  private:
    EbEventBase* _new_event  ( const EbBitMask& );
    bool         _is_complete( EbEventBase*, const EbBitMask& );
  private:
    GenericPool _keys;
    unsigned    _no_builds[Sequence::NumberOfTypes];
  };
}
#endif
