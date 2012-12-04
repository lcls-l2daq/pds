#include "pds/camera/TM6740Manager.hh"

#include "pds/camera/CameraDriver.hh"
#include "pds/camera/CameraBase.hh"
#include "pds/camera/FexFrameServer.hh"

#include "pds/config/TM6740ConfigType.hh"
#include "pds/config/CfgCache.hh"

#include <stdio.h>

namespace Pds {
  class TM6740Config : public CfgCache {
  public:
    TM6740Config(const Src& src) :
      CfgCache(src, _tm6740ConfigType, sizeof(TM6740ConfigType)) {}
  private:
    int _size(void* tc) const 
    { return sizeof(TM6740ConfigType); }
  };
};

using namespace Pds;


TM6740Manager::TM6740Manager(const Src& src) :
  FexCameraManager(src, new TM6740Config(src))
{
}

TM6740Manager::~TM6740Manager()
{
}

void TM6740Manager::_configure(const void* buff)
{
  driver().camera().set_config_data(buff);
}  
