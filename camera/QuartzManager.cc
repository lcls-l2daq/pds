#include "pds/camera/QuartzManager.hh"

#include "pds/camera/CameraDriver.hh"
#include "pds/camera/CameraBase.hh"
#include "pds/camera/FexFrameServer.hh"

#include "pds/config/QuartzConfigType.hh"
#include "pds/config/CfgCache.hh"

#include "pdsdata/camera/FrameCoord.hh"

#include <stdio.h>

using namespace Pds;


static const int MaxConfigSize = 
  sizeof(QuartzConfigType)+ 
  QuartzConfigType::LUT_Size*sizeof(uint16_t) +
  1000*sizeof(Camera::FrameCoord);

namespace Pds {
  class QuartzConfig : public CfgCache {
  public:
    QuartzConfig(const Src& src) :
      CfgCache(src, _quartzConfigType, MaxConfigSize) {}
  private:
    int _size(void* tc) const { return reinterpret_cast<QuartzConfigType*>(tc)->size(); }
  };
};

QuartzManager::QuartzManager(const Src&  src) :
  FexCameraManager(src, new QuartzConfig(src))
{
}

QuartzManager::~QuartzManager()
{
}

void QuartzManager::_configure(const void* buff)
{
  driver().camera().set_config_data(buff);
}  
