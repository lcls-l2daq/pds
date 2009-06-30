#include "pds/camera/Opal1kManager.hh"

#include "pds/camera/Opal1kCamera.hh"
#include "pds/camera/FexFrameServer.hh"

#include "pds/config/Opal1kConfigType.hh"

#include "pds/service/GenericPool.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/OccurrenceId.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/Datagram.hh"
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


Opal1kManager::Opal1kManager(const Src& src) :
  CameraManager(src, MaxConfigSize),
  _camera    (new PdsLeutron::Opal1kCamera),
  _configdata(0),
  _configtc  (_opal1kConfigType, src, (1<<Damage::UserDefined)),
  _occPool   (new GenericPool(sizeof(Occurrence),1))
{
}

Opal1kManager::~Opal1kManager()
{
  delete   _camera;
  delete   _occPool;
}

void Opal1kManager::_configure(char* buff)
{
  _configtc.damage = 0;
  _configdata = new(buff) Opal1kConfigType();
  _camera->Config(*_configdata);
  server().setCameraOffset(_configdata->output_offset());
}  

void Opal1kManager::_configure(InDatagram* in)
{
  _configtc.extent = sizeof(Xtc);
  if (_configtc.damage.value())
    in->datagram().xtc.damage.increase(_configtc.damage.value());
  else {
    _configtc.extent += _configdata->size();  
    in->insert(_configtc, _configdata);
  }
  _configtc.damage.increase(Damage::UserDefined);
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
}

PdsLeutron::PicPortCL& Opal1kManager::camera() { return *_camera; }

const TypeId& Opal1kManager::camConfigType() { return _opal1kConfigType; }
