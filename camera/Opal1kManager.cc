#include "pds/camera/Opal1kManager.hh"

#include "pds/config/FrameFexConfigType.hh"
#include "pds/config/Opal1kConfigType.hh"
#include "pds/config/CfgCache.hh"

#include "pds/camera/CameraBase.hh"
#include "pds/camera/CameraDriver.hh"
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

  class FexConfig : public CfgCache {
  public:
    FexConfig(const Src& src) :
      CfgCache(src,_frameFexConfigType,sizeof(FrameFexConfigType)) {}
  private:
    int _size(void* tc) const { return reinterpret_cast<FrameFexConfigType*>(tc)->size(); }
  };
};

Opal1kManager::Opal1kManager(const Src&  src) :
  CameraManager(src, new Opal1kConfig(src)),
  _fexConfig   (new FexConfig(src)),
  _server      (new FexFrameServer(src)),
  _occPool     (new GenericPool(sizeof(Occurrence),1))
{
}

Opal1kManager::~Opal1kManager()
{
  delete   _occPool;
  delete   _server;
  delete   _fexConfig;
}

FrameServer& Opal1kManager::server() { return *_server; }

void Opal1kManager::allocate (Transition* tr)
{
  CameraManager::allocate(tr);

  const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
  _fexConfig->init(alloc.allocation());
}

void Opal1kManager::doConfigure(Transition* tr)
{
  CameraManager::doConfigure(tr);

  if (_fexConfig->fetch(tr) > 0) {
    //
    //  The feature extraction needs to know 
    //  something about the camera output:
    //     offset, defective pixels, ...
    //
    _server->setFexConfig(*reinterpret_cast<const FrameFexConfigType*>(_fexConfig->current()));
  }
  else {
    printf("Config::configure failed to retrieve FrameFex configuration\n");
    _fexConfig->damage().increase(Damage::UserDefined);
  }
}

void Opal1kManager::nextConfigure    (Transition* tr)
{
  CameraManager::nextConfigure(tr);
  _fexConfig->next();
}

InDatagram* Opal1kManager::recordConfigure  (InDatagram* in) 
{
  _fexConfig->record(CameraManager::recordConfigure(in));
  return in;
}

void Opal1kManager::_configure(const void* buff)
{
  const Opal1kConfigType& c = *reinterpret_cast<const Opal1kConfigType*>(buff);
  driver().camera().set_config_data(buff);
  _server->setCameraOffset(c.output_offset());
}  
