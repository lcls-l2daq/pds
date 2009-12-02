#ifndef PDS_EBC_HH
#define PDS_EBC_HH

#include "Eb.hh"

#include "pds/service/GenericPool.hh"

namespace Pds {

class EbC : public Eb
  {
  public:
    EbC(const Src& id,
	const TypeId& ctns,
	Level::Type level,
	Inlet& inlet,
	OutletWire& outlet,
	int stream,
	int ipaddress,
	unsigned eventsize,
	unsigned eventpooldepth,
	VmonEb* vmoneb=0);
    ~EbC();
  private:
    EbEventBase* _new_event  ( const EbBitMask& );
    EbEventBase* _new_event  ( const EbBitMask&, char* payload, unsigned sizeofPayload );
  private:
    GenericPool _keys;
  };
}
#endif
