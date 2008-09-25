#ifndef PDS_EBBASE_HH
#define PDS_EBBASE_HH

//#define USE_VMON

#include "InletWireServer.hh"
#include "EbDummyTC.hh"
#include "EbEventBase.hh"
#include "EbHeaderTC.hh"
#include "EbTimeouts.hh"
#include "pds/service/LinkedList.hh"

#ifdef USE_VMON
#include "VmonEb.hh"
#endif

namespace Pds {

class OutletWire;
class EbServer;
class Client;
class Appliance;

class EbBase : public InletWireServer
  {
  public:
    EbBase(const Src& id,
	   Level::Type level,
	   Inlet& inlet,
	   OutletWire& outlet,
	   int stream,
	   int ipaddress,
#ifdef USE_VMON
	   const VmonEb& vmoneb,
#endif
	   const Ins* dstack=0);
    virtual ~EbBase();
  public:
    // Implements InletWireServer for adding and removing servers
    Server*   accept (Server*);
    void      remove (unsigned id);
    void      flush  ();
  public:
    int  poll      ();
    int  processTmo();
    void dump      (int);
  private:
    friend class serverRundown;
  protected:
    //  Helper functions for the complete implementation
    EbBitMask    _postEvent(EbEventBase*);  // complete this event and all older than
    EbEventBase* _seek     (EbServer*);
    EbBitMask    _armMask  (EbEventBase*,EbEventBase*);
    EbEventBase* _event    (EbServer* server);
    void         _iterate_dump();
  private:
    void         _post     (EbEventBase*);  // complete this event
    void         _remove   (EbServer*);
  protected:
    virtual void         _fixup      ( EbEventBase*, const Src& ) = 0;
    virtual EbEventBase* _new_event  ( const EbBitMask& ) = 0;
    virtual bool         _is_complete( EbEventBase*, const EbBitMask& );
    virtual void         _dump       ( int detail ) = 0;
  protected:
    LinkedList<EbEventBase> _pending;      // Under construction/completion queue
    EbHeaderTC  _header;       // Template for TC of built events
    EbDummyTC   _dummy;        // Template for TC of dummy contributions
  protected:
    EbBitMask   _clients;      // Database of clients
    EbBitMask   _valued_clients; // Database of clients worthy of receiving
    EbTimeouts  _ebtimeouts;
    Appliance&  _output;       // Destination for datagrams
    Src         _id;           // Our OWN ID
    unsigned    _nobuilds;     // 
    unsigned    _hits;         // # of contributions received and consumed.
    unsigned    _segments;     // # of segments received.
    unsigned    _misses;       // # of cache misses
    unsigned    _discards;     // # of discards due to aged datagram
    Client*     _ack;          // connected port to send ack on.
#ifdef USE_VMON
    VmonEb      _vmoneb;
#endif
  };
}
#endif
