#include "pds/camera/FexCameraManager.hh"
#include "pds/camera/FexFrameServer.hh"

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

void FexCameraManager::doConfigure(Transition* tr)
{
  CameraManager::doConfigure(tr);
  _server      ->doConfigure(tr);
}

void FexCameraManager::nextConfigure    (Transition* tr)
{
  CameraManager::nextConfigure(tr);
  _server      ->nextConfigure(tr);
}

InDatagram* FexCameraManager::recordConfigure  (InDatagram* in) 
{
  return _server->recordConfigure(CameraManager::recordConfigure(in));
}

