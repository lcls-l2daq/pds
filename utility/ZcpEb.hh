#ifndef Pds_ZcpEb_hh
#define Pds_ZcpEb_hh

#include "EbBase.hh"

#include "ZcpEbEvent.hh"

#include "pds/service/GenericPoolW.hh"

namespace Pds {

  class ZcpEb : public EbBase {
  public:
    ZcpEb(const Src& id,
    const TypeId& ctns,
    Level::Type level,
    Inlet& inlet,
    OutletWire& outlet,
    int stream,
    int ipaddress,
    unsigned eventsize,
    unsigned eventpooldepth,
    int slowEb,
    VmonEb* vmoneb,
    const Ins* dstack=0);
    virtual ~ZcpEb();
  public:
    int processIo (Server*);
  private:
    unsigned     _fixup      ( EbEventBase*, const Src&, const EbBitMask& );
    void         _dump       ( int detail );
  protected:
    GenericPoolW _datagrams;    // Datagram freelist
    GenericPool  _events;
    ZcpFragment  _zfragment;   // Datagram freelist
  };
}

#endif
