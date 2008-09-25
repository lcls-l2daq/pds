#ifndef Pds_ZcpEb_hh
#define Pds_ZcpEb_hh

#include "EbBase.hh"

#include "ZcpEbEvent.hh"

namespace Pds {

  class ZcpEb : public EbBase {
  public:
    ZcpEb(const Src& id,
	  Level::Type level,
	  Inlet& inlet,
	  OutletWire& outlet,
	  int stream,
	  int ipaddress,
	  unsigned eventsize,
	  unsigned eventpooldepth,
#ifdef USE_VMON
	  const VmonEb& vmoneb,
#endif
	  const Ins* dstack=0);
    virtual ~ZcpEb();
  public:
    int processIo (Server*);
  private:
    void         _fixup      ( EbEventBase*, const Src& );
    void         _dump       ( int detail );
  protected:
    GenericPool _datagrams;    // Datagram freelist
    GenericPool _zfragments;   // Datagram freelist
    GenericPool _events;
  };
}

#endif
