#include "pds/camera/TM6740Manager.hh"

#include "pds/camera/TM6740Camera.hh"
#include "pds/camera/FexFrameServer.hh"

#include "pds/config/TM6740ConfigType.hh"
#include "pds/config/CfgCache.hh"

#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/Datagram.hh"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <new>

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
  CameraManager(src, new TM6740Config(src)),
  _camera    (new PdsLeutron::TM6740Camera)
{
}

TM6740Manager::~TM6740Manager()
{
  delete   _camera;
}

void TM6740Manager::_configure(const void* buff)
{
  const TM6740ConfigType& c = *reinterpret_cast<const TM6740ConfigType*>(buff);
  _camera->Config(c);
  server().setCameraOffset(c.vref());
}  

PdsLeutron::PicPortCL& TM6740Manager::camera() { return *_camera; }
