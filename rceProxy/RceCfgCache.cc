#include "RceCfgCache.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pdsdata/xtc/Damage.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pds/config/pnCCDConfigType.hh"
#include "pds/config/CsPadConfigType.hh"
#include "pds/utility/Transition.hh"
#include "pds/xtc/InDatagram.hh"

using namespace Pds;

RceCfgCache::RceCfgCache(CfgClientNfs& cfg,
			 const DetInfo::Device dev) : 
  _cfg       (cfg),
  _cur_config(0),
  _end_config(0),
  _scan      (false)
{
  _cfgtc.src = cfg.src();
  switch(dev) {
  case DetInfo::pnCCD :
    _cfgtc.extent   = sizeof(Xtc)+sizeof(pnCCDConfigType);
    _cfgtc.contains = TypeId(TypeId::Id_pnCCDconfig, 2);
    _bsize          = sizeof(pnCCDConfigType); 
    break;
  case DetInfo::Cspad :
    _cfgtc.extent   = sizeof(Xtc)+sizeof(CsPadConfigType);
    _cfgtc.contains = TypeId(TypeId::Id_CspadConfig, 1);
    _bsize          = 100*sizeof(CsPadConfigType); 
    break;
  default :
    printf("RceCfgCache: Bad device index! %u\n", dev);
  }
  _buffer = new char[_bsize];
}

RceCfgCache::~RceCfgCache() { delete[] _buffer; }

const Src& RceCfgCache::src       () const { return _cfg.src(); }

const Xtc& RceCfgCache::tc        () const { return _cfgtc; }

bool       RceCfgCache::scan      () const { return _scan; }

Damage&    RceCfgCache::damage    ()       { return _cfgtc.damage; }

void       RceCfgCache::initialize(const Allocation& alloc) { _cfg.initialize(alloc); }

int        RceCfgCache::fetch     (const Transition& tr)
{
  _cur_config = 0;
  int len = _cfg.fetch(tr, _cfgtc.contains, _buffer, _bsize);
  if (len > 0) {
    printf("RceCfgCache: configuration is size 0x%x bytes\n",len);
    _cur_config = _buffer;
    _end_config = _buffer + len;
    _cfgtc.damage = 0;
    _scan = (len > _cfgtc.sizeofPayload());
  }
  else
    _cfgtc.damage.increase(Damage::UserDefined);

  return len;
}

const void* RceCfgCache::current() const { return _cur_config; }

void RceCfgCache::next()
{
  if (_cur_config) {
    char* nxt_config = _cur_config+_cfgtc.sizeofPayload();
    if (nxt_config < _end_config)
      _cur_config = nxt_config;
    else
      _cur_config = _buffer;
    _cfgtc.damage = 0;
  }
}

void* RceCfgCache::allocate()
{
  _cur_config = _buffer;
  _end_config = _buffer + _cfgtc.sizeofPayload();
  _cfgtc.damage = 0;
  return _buffer;
}

void RceCfgCache::record(InDatagram* dg) const
{
  if (_cfgtc.damage.value())
    dg->datagram().xtc.damage.increase(_cfgtc.damage.value());
  else {
    dg->insert(_cfgtc, _cur_config);
  }
}

