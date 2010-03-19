#ifndef PDS_EBS_HH
#define PDS_EBS_HH

#include "Eb.hh"

#include "pds/service/GenericPool.hh"

namespace Pds {

class EbS : public Eb
  {
  public:
    EbS(const Src& id,
	const TypeId& ctns,
	Level::Type level,
	Inlet& inlet,
	OutletWire& outlet,
	int stream,
	int ipaddress,
	unsigned eventsize,
	unsigned eventpooldepth,
	VmonEb* vmoneb=0);
    ~EbS();

    void no_build(Sequence::Type type, unsigned mask);
  private:
    EbEventBase* _new_event  ( const EbBitMask& );
    EbEventBase* _new_event  ( const EbBitMask&, char* payload, unsigned sizeofPayload );
    unsigned     _fixup      ( EbEventBase*, const Src&, const EbBitMask& );
    IsComplete   _is_complete( EbEventBase*, const EbBitMask& );
  protected:
    GenericPool _keys;
    unsigned    _no_builds[Sequence::NumberOfTypes];
  };
}
#endif
