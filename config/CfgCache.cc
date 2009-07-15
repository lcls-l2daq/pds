#include "CfgCache.hh"

#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/Datagram.hh"

#include <stdio.h>

using namespace Pds;


CfgCache::CfgCache(const Src&    src,
		   const TypeId& id,
		   int           size) :
  _config    (src),
  _type      (id),
  _configtc  (id, src),
  _buffer    (new char[size]),
  _cur_config(0),
  _end_config(0),
  _changed   (false)
{
}


CfgCache::~CfgCache()
{
  delete[] _buffer;
}

void        CfgCache::init   (const Allocation& alloc)
{ _config.initialize(alloc); }

int CfgCache::fetch  (Transition* tr)
{
  _cur_config = 0;
  _changed = true;
  int len = _config.fetch(*tr, _type, _buffer);
  if (len > 0) {
    printf("CfgCache: configuration is size 0x%x bytes\n",len);
    _cur_config = _buffer;
    _end_config = _buffer + len;
    _configtc.damage = 0;
    _configtc.extent = sizeof(Xtc) + _size(_cur_config);
  }
  else
    _configtc.damage.increase(Damage::UserDefined);

  return len;
}

bool CfgCache::changed() const { return _changed; }

const void* CfgCache::current() const
{
  return _cur_config;
}


void CfgCache::next()
{
  char* cur_config = _cur_config;
  if (_cur_config) {
    char* nxt_config = _cur_config+_size(_cur_config);
    if (nxt_config < _end_config)
      _cur_config = nxt_config;
    else
      _cur_config = _buffer;
    _configtc.damage = 0;
    _configtc.extent = sizeof(Xtc) + _size(_cur_config);
  }
  if (cur_config != _cur_config)
    _changed = true;
}


Damage& CfgCache::damage() { return _configtc.damage; }

void CfgCache::record(InDatagram* dg) const
{
  if (_changed) {
    if (_configtc.damage.value())
      dg->datagram().xtc.damage.increase(_configtc.damage.value());
    else {
      dg->insert(_configtc, _cur_config);
    }
    CfgCache& p = const_cast<CfgCache&>(*this);
    p._changed = false;
  }
}


