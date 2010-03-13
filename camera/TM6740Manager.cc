#include "pds/camera/TM6740Manager.hh"

#include "pds/camera/TM6740Camera.hh"

#include "pds/config/FrameFexConfigType.hh"
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

  class FexConfig : public CfgCache {
  public:
    FexConfig(const Src& src) :
      CfgCache(src,_frameFexConfigType,sizeof(FrameFexConfigType)) {}
  private:
    int _size(void* tc) const { return reinterpret_cast<FrameFexConfigType*>(tc)->size(); }
  };
};

using namespace Pds;


TM6740Manager::TM6740Manager(const Src& src) :
  CameraManager(src, new TM6740Config(src)),
  _fexConfig   (new FexConfig(src)),
  _server      (new FexFrameServer(src,*_splice)),
  _camera    (0)
{
}

TM6740Manager::~TM6740Manager()
{
  delete   _server;
  delete   _fexConfig;
}

FrameServer& TM6740Manager::server() { return *_server; }

void TM6740Manager::attach_camera() { _camera = new PdsLeutron::TM6740Camera; }
void TM6740Manager::detach_camera() { delete _camera; }

void TM6740Manager::allocate (Transition* tr)
{
  CameraManager::allocate(tr);

  const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
  _fexConfig->init(alloc.allocation());
}

void TM6740Manager::doConfigure(Transition* tr)
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

void TM6740Manager::nextConfigure    (Transition* tr)
{
  CameraManager::nextConfigure(tr);
  _fexConfig->next();
}

InDatagram* TM6740Manager::recordConfigure  (InDatagram* in) 
{
  _fexConfig->record(CameraManager::recordConfigure(in));
  return in;
}

void TM6740Manager::_configure(const void* buff)
{
  const TM6740ConfigType& c = *reinterpret_cast<const TM6740ConfigType*>(buff);
  _camera->Config(c);
  _server->setCameraOffset(c.vref());
}  

PdsLeutron::PicPortCL& TM6740Manager::camera() { return *_camera; }
