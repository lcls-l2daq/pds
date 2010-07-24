#include "pds/camera/FccdManager.hh"

#include "pds/camera/FccdCamera.hh"

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

FccdManager::FccdManager(const Src& src,
           unsigned   grabberId) :
  CameraManager(src, new FccdConfig(src)),
  _server      (new FccdFrameServer(src,*_splice)),
  _camera    (0),
  _occPool     (new GenericPool(sizeof(Occurrence),1)),
  _grabberId (grabberId)
{
}

FccdManager::~FccdManager()
{
  delete   _occPool;
  delete   _server;
}

FrameServer& FccdManager::server() { return *_server; }

void FccdManager::attach_camera() { _camera = new PdsLeutron::FccdCamera(NULL,_grabberId); }
void FccdManager::detach_camera() { delete _camera; }

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
  const FccdConfigType& c = *reinterpret_cast<const FccdConfigType*>(buff);
  if (_camera) {
    _camera->Config(c);
  } else {
    printf("Error: _camera is NULL at %s line %d\n", __FILE__, __LINE__);
  }
}  

Pds::Damage FccdManager::_handle()
{
  //  Trigger a clear readout
//   if (_camera->CurrentCount == 2001)
//     _outOfOrder = false;
//   if (_camera->CurrentCount == 2000)
//     _camera->CurrentCount++;

  // FCCD frame counter is 16-bit, so limit comparison to 16-bit
  if (!_outOfOrder &&
      ((0xffff & _camera->CurrentCount) != (0xffff & _nposts))) {
    _outOfOrder = true;

    Pds::Occurrence* occ = new (_occPool)
      Pds::Occurrence(Pds::OccurrenceId::ClearReadout);
    appliance().post(occ);

    printf("Camera frame number(%ld) != Server number(%d)\n",
     0xffff & _camera->CurrentCount, 0xffff & _nposts);
  }

  return Pds::Damage(_outOfOrder ? (1<<Pds::Damage::OutOfOrder) : 0);
}

void FccdManager::_register()
{
  _outOfOrder = false;
}

void FccdManager::_unregister()
{
#if 0
  printf("=== Fccd dump ===\n nposts %d\n",_nposts);
  _camera->dump();
  printf("buffered frame ids: ");
  for(unsigned k=0; k<16; k++) {
    PdsLeutron::FrameHandle* hdl = _camera->GetFrameHandle();
    unsigned short *data = (unsigned short *)hdl->data;
    unsigned Count = (data[0]<<24) | (data[1]<<16) | (data[2]<<8) | data[3];
    printf("%p/%p/%d ",hdl->data, hdl->arg, Count);
    delete hdl;
  }
  printf("\n");
#endif
}

PdsLeutron::PicPortCL& FccdManager::camera()
 { return *_camera; }
