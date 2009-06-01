#include "pds/camera/TM6740Manager.hh"

#include "pds/camera/TM6740Camera.hh"
#include "pds/camera/FexFrameServer.hh"

#include "pds/config/TM6740ConfigType.hh"

#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/Datagram.hh"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <new>

using namespace Pds;


static const int MaxConfigSize = sizeof(TM6740ConfigType);

TM6740Manager::TM6740Manager(const Src& src) :
  CameraManager(src, MaxConfigSize),
  _camera    (new PdsLeutron::TM6740Camera),
  _configdata(0),
  _configtc  (_tm6740ConfigType, src, (1<<Damage::UserDefined))
{
}

TM6740Manager::~TM6740Manager()
{
  delete   _camera;
}

void TM6740Manager::_configure(char* buff)
{
  _configtc.damage = 0;
  _configdata = new(buff) TM6740ConfigType;
  _camera->Config(*_configdata);
  server().setCameraOffset(_configdata->vref());
}  

void TM6740Manager::_configure(InDatagram* in)
{
  _configtc.extent = sizeof(Xtc);
  if (_configtc.damage.value())
    in->datagram().xtc.damage.increase(_configtc.damage.value());
  else {
    _configtc.extent += sizeof(_configdata);
    in->insert(_configtc, _configdata);
  }
  _configtc.damage.increase(Damage::UserDefined);
}

PdsLeutron::PicPortCL& TM6740Manager::camera() { return *_camera; }

const TypeId& TM6740Manager::camConfigType() { return _tm6740ConfigType; }
