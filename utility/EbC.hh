#ifndef PDS_EBC_HH
#define PDS_EBC_HH

#include "Eb.hh"

#include "pds/service/GenericPool.hh"

namespace Pds {

class EbC : public Eb
  {
  public:
    EbC(const Src& id,
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
    ~EbC();
  private:
    EbEventBase* _new_event  ( const EbBitMask& );
  private:
    GenericPool _keys;
  };
}
#endif
