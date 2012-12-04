#include "pds/camera/OrcaManager.hh"

#include "pds/camera/CameraDriver.hh"
#include "pds/camera/CameraBase.hh"
#include "pds/camera/FexFrameServer.hh"

#include "pds/config/OrcaConfigType.hh"
#include "pds/config/CfgCache.hh"

#include <stdio.h>

using namespace Pds;

static const int MaxConfigSize = sizeof(OrcaConfigType);

namespace Pds {
  class OrcaConfig : public CfgCache {
  public:
    OrcaConfig(const Src& src) :
      CfgCache(src, _orcaConfigType, MaxConfigSize) {}
  private:
    int _size(void* tc) const { return reinterpret_cast<OrcaConfigType*>(tc)->size(); }
  };
};

OrcaManager::OrcaManager(const Src&  src) :
  FexCameraManager(src, new OrcaConfig(src))
{
}

OrcaManager::~OrcaManager()
{
}

void OrcaManager::_configure(const void* buff)
{
  const OrcaConfigType& c = *reinterpret_cast<const OrcaConfigType*>(buff);
  driver().camera().set_config_data(buff);
}  
