#ifndef Pds_FastSegWire_hh
#define Pds_FastSegWire_hh

#include "pds/utility/SegWireSettings.hh"

#include <list>

namespace Pds {
  class EbServer;

  class FastSegWire : public SegWireSettings {
  public:
    FastSegWire(EbServer&   server,
                unsigned    paddr,
                const char* alias        =0,
                unsigned    maxeventsize =(1<<20),
                unsigned    maxeventdepth=64,
                unsigned    maxeventtime =(1<<16));
    FastSegWire(std::list<EbServer*>,
                unsigned    paddr,
                const char* alias        =0,
                unsigned    maxeventsize =(1<<20),
                unsigned    maxeventdepth=64,
                unsigned    maxeventtime =(1<<16));
    ~FastSegWire();
  public:
    void connect (InletWire& wire,
		  StreamParams::StreamType s,
		  int interface);
    const std::list<Src>&      sources () const;
    const std::list<SrcAlias>* pAliases() const;
    bool     needs_evr      () const;
    bool     is_triggered   () const;
    unsigned module         () const;
    unsigned channel        () const;
    unsigned max_event_size () const;
    unsigned max_event_depth() const;
    unsigned max_event_time () const;
    unsigned paddr          () const;
  private:
    std::list<EbServer*> _server;
    std::list<Src>       _sources;
    std::list<SrcAlias>  _aliases;
    unsigned             _maxeventsize;    
    unsigned             _maxeventdepth;
    unsigned             _maxeventtime;
    unsigned             _paddr;
  };
};

#endif
