//! Camera.cc
//! See Camera.hh for a description of the Camera class.
//!
//! Copyright 2008, SLAC
//! Author: rmachet@slac.stanford.edu
//! GPL license
//!

#include "FrameHandle.hh"
#include <stdio.h>
#include <errno.h>
#include <string.h>

using namespace PdsLeutron;

FrameHandle::FrameHandle(unsigned long _width, unsigned long _height, 
			 enum FrameHandle::Format _format, void *_data):
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

FrameHandle::FrameHandle(unsigned long _width, unsigned long _height, 
			 enum Format _format, int _elsize, void *_data):
      width(_width), height(_height), format(_format), elsize(_elsize), data(_data),
      release(NULL), obj(NULL), arg(NULL) {

}

FrameHandle::FrameHandle(unsigned long _width, unsigned long _height, 
			 enum Format _format, int _elsize, void *_data,
			 void(*_release)(void * obj, FrameHandle *pFrameHandle, 					 void *arg), void *_obj, void *_arg):
      width(_width), height(_height), format(_format), 
      elsize(_elsize), data(_data),
      release(_release), obj(_obj), arg(_arg) 
{
}

FrameHandle::~FrameHandle() 
{
  if (release != NULL)
    release(obj, this, arg);
}

unsigned FrameHandle::depth() const
{
  unsigned d = 8;
  switch(format) {
  case FORMAT_GRAYSCALE_8 : d =  8; break;
  case FORMAT_GRAYSCALE_10: d = 10; break;
  case FORMAT_GRAYSCALE_12: d = 12; break;
  case FORMAT_GRAYSCALE_16: d = 16; break;
  case FORMAT_RGB_32      : d = 32; break;
  }
  return d;
}
