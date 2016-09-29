#ifndef PDS_EB_HH
#define PDS_EB_HH

#include "EbBase.hh"

#include "EbEvent.hh"
#include "EbTimeouts.hh"

#include "pds/service/GenericPoolW.hh"

namespace Pds {

class OutletWire;
class EbServer;
class Client;

class Eb : public EbBase
  {
  public:
    Eb(const Src& id,
       const TypeId& ctns,
       Level::Type level,
       Inlet& inlet,
       OutletWire& outlet,
       int stream,
       int ipaddress,
       unsigned eventsize,
       unsigned eventpooldepth,
       VmonEb* vmoneb,
       const Ins* dstack=0);
    virtual ~Eb();
  public:
    int  processIo(Server*);
  private:
    unsigned     _fixup      ( EbEventBase*, const Src&, const EbBitMask& );
    void         _insert     ( EbEventBase* );
    void         _dump       ( int detail );
  protected:
    GenericPoolW _datagrams;    // Datagram freelist : This needs to be thread-safe, but don't need blocking on empty; can use atomics for counter increments
    GenericPool  _events;       // This doesn't need to be thread-safe!
  };
}
#endif
