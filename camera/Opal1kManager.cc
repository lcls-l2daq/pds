#include "pds/camera/Opal1kManager.hh"

#include "pds/camera/CameraDriver.hh"
#include "pds/camera/CameraBase.hh"
#include "pds/camera/FexFrameServer.hh"

#include "pds/config/Opal1kConfigType.hh"
#include "pds/config/CfgCache.hh"

#include "pdsdata/camera/FrameCoord.hh"

#include <stdio.h>

using namespace Pds;


static const int MaxConfigSize = 
  sizeof(Opal1kConfigType)+ 
  Opal1kConfigType::LUT_Size*sizeof(uint16_t) +
  1000*sizeof(Camera::FrameCoord);

namespace Pds {
  class Opal1kConfig : public CfgCache {
  public:
    Opal1kConfig(const Src& src) :
      CfgCache(src, _opal1kConfigType, MaxConfigSize) {}
  private:
    int _size(void* tc) const { return reinterpret_cast<Opal1kConfigType*>(tc)->size(); }
  };
};

Opal1kManager::Opal1kManager(const Src&  src) :
  FexCameraManager(src, new Opal1kConfig(src))
{
}

Opal1kManager::~Opal1kManager()
{
}

void Opal1kManager::_configure(const void* buff)
{
  driver().camera().set_config_data(buff);
}  
