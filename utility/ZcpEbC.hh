#ifndef PDS_ZCPEBC_HH
#define PDS_ZCPEBC_HH

#include "ZcpEb.hh"

#include "pds/service/GenericPool.hh"

namespace Pds {

class ZcpEbC : public ZcpEb
  {
  public:
    ZcpEbC(const Src& id,
     const TypeId& ctns,
     Level::Type level,
     Inlet& inlet,
     OutletWire& outlet,
     int stream,
     int ipaddress,
     unsigned eventsize,
     unsigned eventpooldepth,
     int slowEb,
     VmonEb* vmoneb=0);
    ~ZcpEbC();
  private:
    EbEventBase* _new_event  ( const EbBitMask& );
  private:
    GenericPool _keys;
  };
}
#endif
