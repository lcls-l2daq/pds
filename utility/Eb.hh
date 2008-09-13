#ifndef PDS_EB_HH
#define PDS_EB_HH

//#define USE_VMON

#include "EbBase.hh"

#include "EbEvent.hh"
#include "EbHeaderTC.hh"
#include "EbTimeouts.hh"
#ifdef USE_VMON
#include "VmonEb.hh"
#endif

namespace Pds {

class OutletWire;
class EbServer;
class Client;

class Eb : public EbBase 
  {
  public:
    Eb(const Src& id,
       Level::Type level,
       Inlet& inlet,
       OutletWire& outlet,
       int stream,
       int ipaddress,
       unsigned eventsize,
       unsigned eventpooldepth,
       unsigned netbufdepth,
       const EbTimeouts& ebtimeouts,
#ifdef USE_VMON
       const VmonEb& vmoneb,
#endif
       const Ins* dstack=0);
    ~Eb();
  public:
    int  processIo(Server*);
  private:
    void         _fixup      ( EbEventBase*, const Src& );
    EbEventBase* _new_event  ( const EbBitMask& );
    void         _dump       ( int detail );
  private:
    GenericPool _datagrams;    // Datagram freelist
    GenericPool _events;       // Event freelist
  };
}
#endif
