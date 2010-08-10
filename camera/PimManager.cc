#include "pds/camera/PimManager.hh"

#include "pds/config/CfgCache.hh"
#include "pds/config/PimImageConfigType.hh"

#include <stdio.h>

namespace Pds {
  class FexConfig : public CfgCache {
  public:
    FexConfig(const Src& src) :
      CfgCache(src,_pimImageConfigType,sizeof(PimImageConfigType)) {}
  private:
    int _size(void* tc) const { return reinterpret_cast<FrameFexConfigType*>(tc)->size(); }
  };
};

using namespace Pds;


PimManager::PimManager(const Src& src, unsigned grabberId) :
  TM6740Manager(src, grabberId),
  _fexConfig    (new FexConfig(src))
{
}

PimManager::~PimManager()
{
  delete   _fexConfig;
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
