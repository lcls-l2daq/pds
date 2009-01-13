//! Camera.cc
//! See Camera.hh for a description of the Camera class.
//!
//! Copyright 2008, SLAC
//! Author: rmachet@slac.stanford.edu
//! GPL license
//!

#include "Camera.hh"
#include <stdio.h>
#include <errno.h>
#include <string.h>

using namespace PdsLeutron;

Camera::Camera() {
  pExtraConfig = NULL;
  lenExtraConfig = 0;
  status.CameraId = "none";
  status.CameraName = "default camera";
  status.CapturedFrames = 0;
  status.DroppedFrames = 0;
  status.State = CAMERA_UNCONFIGURED;
}

Camera::~Camera() {
}

int Camera::SetNotification(enum NotifyType mode) { 
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

int Camera::SetConfig(const struct Config &Config) { 
  config = Config;
  // Init must be called again before acquisition can start
  status.State = CAMERA_UNCONFIGURED;
  return 0;
}

int Camera::Init() {
  status.State = CAMERA_READY;
  return 0;
}

int Camera::Start() {
  // Check if Init has been properly called
  if ((status.State != CAMERA_READY) && (status.State != CAMERA_STOPPED))
    return -ENOTCONN;
  status.State = CAMERA_RUNNING;
  return 0; 
}

int Camera::Stop() { 
  // Check if Start has been called
  if (status.State != CAMERA_RUNNING)
    return -ENOTCONN;
  status.State = CAMERA_STOPPED;
  return 0; 
}

int Camera::GetConfig(struct Config &Config) { 
  Config = config;
  return 0;
}

int Camera::GetConfig(struct Config &Config, void *StaticConfigExtra, int StaticConfigExtraSize) { 
  int copylen = lenExtraConfig;

  if (copylen > StaticConfigExtraSize)
    copylen = StaticConfigExtraSize;
  if(StaticConfigExtra != NULL)
    memcpy(StaticConfigExtra, pExtraConfig, copylen);
  Config = config;
  return lenExtraConfig;
}

int Camera::GetStatus(struct Status &Status) {
  Status = status;
  return 0;
}

FrameHandle *Camera::GetFrameHandle() { 
  return (FrameHandle *)NULL; 
}

int Camera::SendCommand(char *szCommand, char *pszResponse, int iResponseBufferSize) { 
  if (pszResponse != NULL)
    *pszResponse=0;
  return 0; 
}
