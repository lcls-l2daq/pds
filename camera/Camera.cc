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

Pds::Frame::Frame(unsigned long _width, unsigned long _height, enum Frame::Format _format, void *_data):
      width(_width), height(_height), format(_format), data(_data),
      release(NULL), obj(NULL), arg(NULL) {
  switch(_format) {
    case FORMAT_GRAYSCALE_8:
      elsize = 1;
      break;
    case FORMAT_GRAYSCALE_10:
    case FORMAT_GRAYSCALE_12:
    case FORMAT_GRAYSCALE_16:
      elsize = 2;
      break;
    case FORMAT_RGB_32:
      elsize = 4;
      break;
  };
}

Pds::Frame::Frame(unsigned long _width, unsigned long _height, enum Format _format, int _elsize, void *_data):
      width(_width), height(_height), format(_format), elsize(_elsize), data(_data),
      release(NULL), obj(NULL), arg(NULL) {

}

Pds::Frame::Frame(unsigned long _width, unsigned long _height, enum Format _format, int _elsize, void *_data,
            void(*_release)(void * obj, Pds::Frame *pFrame, void *arg), void *_obj, void *_arg):
      width(_width), height(_height), format(_format), elsize(_elsize), data(_data),
      release(_release), obj(_obj), arg(_arg) {

}

Pds::Frame::~Frame() {
  if (release != NULL)
    release(obj, this, arg);
}

Pds::Camera::Camera() {
  pExtraConfig = NULL;
  lenExtraConfig = 0;
  status.CameraId = "none";
  status.CameraName = "default camera";
  status.CapturedFrames = 0;
  status.DroppedFrames = 0;
  status.State = CAMERA_UNCONFIGURED;
}

Pds::Camera::~Camera() {
}

int Pds::Camera::SetNotification(enum NotifyType mode) { 
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

int Pds::Camera::SetConfig(const struct Config &Config) { 
  config = Config;
  // Init must be called again before acquisition can start
  status.State = CAMERA_UNCONFIGURED;
  return 0;
}

int Pds::Camera::Init() {
  status.State = CAMERA_READY;
  return 0;
}

int Pds::Camera::Start() {
  // Check if Init has been properly called
  if ((status.State != CAMERA_READY) && (status.State != CAMERA_STOPPED))
    return -ENOTCONN;
  status.State = CAMERA_RUNNING;
  return 0; 
}

int Pds::Camera::Stop() { 
  // Check if Start has been called
  if (status.State != CAMERA_RUNNING)
    return -ENOTCONN;
  status.State = CAMERA_STOPPED;
  return 0; 
}

int Pds::Camera::GetConfig(struct Config &Config) { 
  Config = config;
  return 0;
}

int Pds::Camera::GetConfig(struct Config &Config, void *StaticConfigExtra, int StaticConfigExtraSize) { 
  int copylen = lenExtraConfig;

  if (copylen > StaticConfigExtraSize)
    copylen = StaticConfigExtraSize;
  if(StaticConfigExtra != NULL)
    memcpy(StaticConfigExtra, pExtraConfig, copylen);
  Config = config;
  return lenExtraConfig;
}

int Pds::Camera::GetStatus(struct Status &Status) {
  Status = status;
  return 0;
}

Pds::Frame *Pds::Camera::GetFrame() { 
  return (Pds::Frame *)NULL; 
}

int Pds::Camera::SendCommand(char *szCommand, char *pszResponse, int iResponseBufferSize) { 
  if (pszResponse != NULL)
    *pszResponse=0;
  return 0; 
}
