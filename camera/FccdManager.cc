#include "pds/camera/FccdManager.hh"

#include "pds/camera/FccdCamera.hh"
#include "pds/camera/CameraDriver.hh"

#include "pds/config/FccdConfigType.hh"

#include "pds/config/CfgCache.hh"
#include "pdsdata/camera/FrameCoord.hh"

#include "pds/service/GenericPool.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/OccurrenceId.hh"
#include "pds/utility/Appliance.hh"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <new>

using namespace Pds;

static const int MaxConfigSize = sizeof(FccdConfigType);

namespace Pds {
  class FccdConfig : public CfgCache {
  public:
    FccdConfig(const Src& src) :
      CfgCache(src, _fccdConfigType, MaxConfigSize) {}
  private:
    int _size(void* tc) const { return reinterpret_cast<FccdConfigType*>(tc)->size(); }
  };
};

FccdManager::FccdManager(const Src& src) :
  CameraManager(src, new FccdConfig(src)),
  _server      (new FccdFrameServer(src))
{
}

FccdManager::~FccdManager()
{
  delete   _server;
}

FrameServer& FccdManager::server() { return *_server; }

void FccdManager::allocate (Transition* tr)
{
  CameraManager::allocate(tr);
}

void FccdManager::doConfigure(Transition* tr)
{
  CameraManager::doConfigure(tr);
}

void FccdManager::nextConfigure    (Transition* tr)
{
  CameraManager::nextConfigure(tr);
}

void FccdManager::_configure(const void* buff)
{
  driver().camera().set_config_data(buff);
}  

