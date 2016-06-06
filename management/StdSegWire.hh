#ifndef Pds_StdSegWire_hh
#define Pds_StdSegWire_hh

#include "pds/utility/SegWireSettings.hh"

#include <list>

namespace Pds {
  class EbServer;

  class StdSegWire : public SegWireSettings {
  public:
    StdSegWire(EbServer&   server,
               const char* alias        =0,
               unsigned    maxeventsize =(1<<20),
               unsigned    maxeventdepth=64,
               bool        is_triggered =false,
               unsigned    evr_module   =0,
               unsigned    evr_channel  =0);
    StdSegWire(std::list<EbServer*> server,
               const char* alias        =0,
               unsigned    maxeventsize =(1<<20),
               unsigned    maxeventdepth=64,
               bool        is_triggered =false,
               unsigned    evr_module   =0,
               unsigned    evr_channel  =0);
    ~StdSegWire();
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
  private:
    std::list<EbServer*> _server;
    std::list<Src>       _sources;
    std::list<SrcAlias>  _aliases;
    unsigned             _maxeventsize;    
    unsigned             _maxeventdepth;
    bool                 _istriggered;
    unsigned             _evrmodule;
    unsigned             _evrchannel;
  };
};

#endif
