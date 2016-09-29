#include "pds/management/FastSegWire.hh"

#include "pds/utility/EbServer.hh"
#include "pds/utility/InletWire.hh"

using namespace Pds;

FastSegWire::FastSegWire(EbServer&   server,
                         unsigned    paddr,
                         const char* alias,
                         unsigned    maxeventsize,
                         unsigned    maxeventdepth,
                         unsigned    maxeventtime) :
  _maxeventsize (maxeventsize),
  _maxeventdepth(maxeventdepth),
  _maxeventtime (maxeventtime),
  _paddr        (paddr)
{
  _sources.push_back(server.client()); 

  _server.push_back(&server);

  if (alias) {
    SrcAlias tmpAlias(server.client(), alias);
    _aliases.push_back(tmpAlias);
  }
}

FastSegWire::FastSegWire(std::list<EbServer*> servers,
                         unsigned    paddr,
                         const char* alias,
                         unsigned    maxeventsize,
                         unsigned    maxeventdepth,
                         unsigned    maxeventtime) :
  _server       (servers),
  _maxeventsize (maxeventsize),
  _maxeventdepth(maxeventdepth),
  _maxeventtime (maxeventtime),
  _paddr        (paddr)
{
  for(std::list<EbServer*>::iterator it=servers.begin(); it!=servers.end(); it++)
    _sources.push_back((*it)->client()); 

  if (alias && !servers.empty()) {
    SrcAlias tmpAlias(servers.front()->client(), alias);
    _aliases.push_back(tmpAlias);
  }
}

FastSegWire::~FastSegWire()
{
}

void FastSegWire::connect (InletWire& wire,
                           StreamParams::StreamType s,
                           int interface) {
  for(std::list<EbServer*>::iterator it=_server.begin();
      it!=_server.end(); it++) {
    wire.add_input(*it);
  }
}

const std::list<Src>& FastSegWire::sources() const 
{
  return _sources; 
}

const std::list<SrcAlias>* FastSegWire::pAliases() const
{
  return (_aliases.size() > 0) ? &_aliases : NULL;
}

bool     FastSegWire::needs_evr     () const
{
  return false; 
}

bool     FastSegWire::is_triggered   () const
{
  return false; 
}

unsigned FastSegWire::module         () const
{
  return 0;
}

unsigned FastSegWire::channel        () const
{ 
  return 0;
}

unsigned FastSegWire::max_event_size () const
{
  return _maxeventsize;
}

unsigned FastSegWire::max_event_depth() const
{
  return _maxeventdepth;
}

unsigned FastSegWire::max_event_time() const
{
  return _maxeventtime;
}

unsigned FastSegWire::paddr() const
{
  return _paddr;
}
