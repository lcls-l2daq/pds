#include "pds/camera/FexCameraManager.hh"
#include "pds/camera/FexFrameServer.hh"
#include "pds/camera/CameraDriver.hh"
#include "pds/camera/CameraBase.hh"
#include "pds/utility/Appliance.hh"

using namespace Pds;


FexCameraManager::FexCameraManager(const Src&  src, CfgCache* cache) :
  CameraManager(src, cache),
  _server      (new FexFrameServer(src))
{
}

FexCameraManager::~FexCameraManager()
{
  delete   _server;
}

FrameServer& FexCameraManager::server() { return *_server; }

void FexCameraManager::allocate (Transition* tr)
{
  CameraManager::allocate(tr);
  _server      ->allocate(tr);
}

bool FexCameraManager::doConfigure(Transition* tr)
{
  if (!CameraManager::doConfigure(tr) ||
      !_server      ->doConfigure(tr)) 
    return false;

  UserMessage* msg = _server->validate(driver().camera().camera_width(),
				       driver().camera().camera_height());
  if (msg) {
    appliance().post(msg);
    return false;
  }

  return true;
}

void FexCameraManager::nextConfigure    (Transition* tr)
{
  CameraManager::nextConfigure(tr);
  _server      ->nextConfigure(tr);
  UserMessage* msg = _server->validate(driver().camera().camera_width(),
				       driver().camera().camera_height());
  if (msg)
    appliance().post(msg);
}

InDatagram* FexCameraManager::recordConfigure  (InDatagram* in) 
{
  return _server->recordConfigure(CameraManager::recordConfigure(in));
}

