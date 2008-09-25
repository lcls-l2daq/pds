#ifndef PDS_ZCPEBS_HH
#define PDS_ZCPEBS_HH

#include "ZcpEb.hh"

#include "pds/service/GenericPool.hh"

namespace Pds {

class ZcpEbS : public ZcpEb
  {
  public:
    ZcpEbS(const Src& id,
	   Level::Type level,
	   Inlet& inlet,
	   OutletWire& outlet,
	   int stream,
	   int ipaddress,
	   unsigned eventsize,
	   unsigned eventpooldepth);
#ifdef USE_VMON
    const VmonEb& vmoneb,
#endif
    ~ZcpEbS();
  private:
    EbEventBase* _new_event  ( const EbBitMask& );
  private:
    GenericPool _keys;
  };
}
#endif
