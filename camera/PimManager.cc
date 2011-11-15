#include "pds/camera/PimManager.hh"

#include "pds/config/CfgCache.hh"
#include "pds/config/PimImageConfigType.hh"
#include "pds/utility/Transition.hh"

#include <stdio.h>

namespace Pds {
  class PimConfig : public CfgCache {
  public:
    PimConfig(const Src& src) :
      CfgCache(src,_pimImageConfigType,sizeof(PimImageConfigType)) {}
  private:
    int _size(void* tc) const { return sizeof(PimImageConfigType); }
  };
};

using namespace Pds;


PimManager::PimManager(const Src& src) :
  TM6740Manager(src),
  _fexConfig    (new PimConfig(src))
{
}

PimManager::~PimManager()
{
  delete   _fexConfig;
}

void PimManager::allocate (Transition* tr)
{
  TM6740Manager::allocate(tr);

  const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
  _fexConfig->init(alloc.allocation());
}

void PimManager::doConfigure(Transition* tr)
{
  TM6740Manager::doConfigure(tr);

  if (_fexConfig->fetch(tr) > 0) {
  }
  else {
    printf("Config::configure failed to retrieve PimImage configuration\n");
    _fexConfig->damage().increase(Damage::UserDefined);
  }
}

InDatagram* PimManager::recordConfigure  (InDatagram* in) 
{
  _fexConfig->record(TM6740Manager::recordConfigure(in));
  return in;
}
