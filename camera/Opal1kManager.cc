#include "pds/camera/Opal1kManager.hh"

#include "pds/camera/Opal1kCamera.hh"
#include "pds/camera/FexFrameServer.hh"

#include "pds/config/Opal1kConfigType.hh"
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

Opal1kManager::Opal1kManager(const Src& src) :
  CameraManager(src, new Opal1kConfig(src)),
  _camera    (new PdsLeutron::Opal1kCamera),
  _occPool   (new GenericPool(sizeof(Occurrence),1))
{
}

Opal1kManager::~Opal1kManager()
{
  delete   _camera;
  delete   _occPool;
}

void Opal1kManager::_configure(const void* buff)
{
  const Opal1kConfigType& c = *reinterpret_cast<const Opal1kConfigType*>(buff);
  _camera->Config(c);
  server().setCameraOffset(c.output_offset());
}  

Pds::Damage Opal1kManager::_handle()
{
  //  Trigger a clear readout
//   if (_camera->CurrentCount == 2001)
//     _outOfOrder = false;
//   if (_camera->CurrentCount == 2000)
//     _camera->CurrentCount++;

  if (!_outOfOrder && _camera->CurrentCount != _nposts) {
    _outOfOrder = true;

    Pds::Occurrence* occ = new (_occPool)
      Pds::Occurrence(Pds::OccurrenceId::ClearReadout);
    appliance().post(occ);

    printf("Camera frame number(%ld) != Server number(%d)\n",
	   _camera->CurrentCount, _nposts);
  }

  return Pds::Damage(_outOfOrder ? (1<<Pds::Damage::OutOfOrder) : 0);
}

void Opal1kManager::_register()
{
  _outOfOrder = false;
}

void Opal1kManager::_unregister()
{
#if 0
  printf("=== Opal1k dump ===\n nposts %d\n",_nposts);
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

PdsLeutron::PicPortCL& Opal1kManager::camera() { return *_camera; }
