#include "pds/management/StdSegWire.hh"

#include "pds/utility/EbServer.hh"
#include "pds/utility/InletWire.hh"

using namespace Pds;

StdSegWire::StdSegWire(EbServer&   server,
                       const char* alias,
                       unsigned    maxeventsize,
                       unsigned    maxeventdepth,
                       bool        is_triggered,
                       unsigned    evr_module,
                       unsigned    evr_channel) :
  _maxeventsize (maxeventsize),
  _maxeventdepth(maxeventdepth),
  _istriggered  (is_triggered),
  _evrmodule    (evr_module),
  _evrchannel   (evr_channel)
{
  _sources.push_back(server.client()); 

  _server.push_back(&server);

  if (alias) {
    SrcAlias tmpAlias(server.client(), alias);
    _aliases.push_back(tmpAlias);
  }
}

StdSegWire::StdSegWire(std::list<EbServer*> servers,
                       const char* alias,
                       unsigned    maxeventsize,
                       unsigned    maxeventdepth,
                       bool        is_triggered,
                       unsigned    evr_module,
                       unsigned    evr_channel) :
  _server       (servers),
  _maxeventsize (maxeventsize),
  _maxeventdepth(maxeventdepth),
  _istriggered  (is_triggered),
  _evrmodule    (evr_module),
  _evrchannel   (evr_channel)
{
  for(std::list<EbServer*>::iterator it=servers.begin(); it!=servers.end(); it++)
    _sources.push_back((*it)->client()); 

  if (alias) {
    SrcAlias tmpAlias(servers.front()->client(), alias);
    _aliases.push_back(tmpAlias);
  }
}

StdSegWire::~StdSegWire()
{
}

void StdSegWire::connect (InletWire& wire,
                          StreamParams::StreamType s,
                          int interface) {
  for(std::list<EbServer*>::iterator it=_server.begin();
      it!=_server.end(); it++) {
    wire.add_input(*it);
  }
}

const std::list<Src>& StdSegWire::sources() const 
{
  return _sources; 
}

const std::list<SrcAlias>* StdSegWire::pAliases() const
{
  return (_aliases.size() > 0) ? &_aliases : NULL;
}

bool     StdSegWire::is_triggered   () const
{
  return _istriggered; 
}

unsigned StdSegWire::module         () const
{
  return _evrmodule;
}

unsigned StdSegWire::channel        () const
{ 
  return _evrchannel;
}

unsigned StdSegWire::max_event_size () const
{
  return _maxeventsize;
}

unsigned StdSegWire::max_event_depth() const
{
  return _maxeventdepth;
}
