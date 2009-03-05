//! LvCamera.cc
//! See Camera.hh for a description of the Camera class.
//!
//! Copyright 2008, SLAC
//! Author: rmachet@slac.stanford.edu
//! GPL license
//!

#include "pds/camera/LvCamera.hh"
#include <stdio.h>
#include <errno.h>
#include <string.h>

using namespace PdsLeutron;

LvCamera::LvCamera() {
  status.CameraId = "none";
  status.CameraName = "default camera";
  status.CapturedFrames = 0;
  status.DroppedFrames = 0;
  status.State = CAMERA_UNCONFIGURED;
}

LvCamera::~LvCamera() {
}

int LvCamera::SetNotification(enum NotifyType mode) { 
  int ret;
  switch(mode) {
    case NOTIFYTYPE_NONE:
      ret = 0;
      break;
    default:
      ret = -ENOTSUP;
      break;
  }
  return ret;
}

int LvCamera::ConfigReset() {
  // Init must be called again before acquisition can start
  status.State = CAMERA_UNCONFIGURED;
  return 0;
}

int LvCamera::Init() {
  status.State = CAMERA_READY;
  return 0;
}

int LvCamera::Start() {
  // Check if Init has been properly called
  if ((status.State != CAMERA_READY) && (status.State != CAMERA_STOPPED))
    return -ENOTCONN;
  status.State = CAMERA_RUNNING;
  return 0; 
}

int LvCamera::Stop() { 
  // Check if Start has been called
  if (status.State != CAMERA_RUNNING)
    return -ENOTCONN;
  status.State = CAMERA_STOPPED;
  return 0; 
}

int LvCamera::GetStatus(struct Status &Status) {
  Status = status;
  return 0;
}

