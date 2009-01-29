#include "Frame.hh"
#include "DmaSplice.hh"
#include "FrameHandle.hh"

#include "pdsdata/xtc/Xtc.hh"

#include <string.h>

using namespace Pds;

Frame::Frame(unsigned _width, unsigned _height, unsigned _depth) :
  width (_width),
  height(_height),
  depth (_depth),
  extent(_width*_height*depth_bytes()) 
{
}

Frame::Frame(unsigned _width, unsigned _height, unsigned _depth,
	     const void* _input) :
  width (_width),
  height(_height),
  depth (_depth),
  extent(_width*_height*depth_bytes()) 
{
  memcpy(this+1, _input, extent);
}

Frame::Frame(unsigned _width, unsigned _height, unsigned _depth,
	     PdsLeutron::FrameHandle& handle,
	     DmaSplice&  _splice) :
  width (_width),
  height(_height),
  depth (_depth),
  extent(_width*_height*depth_bytes()) 
{
  _splice.queue(handle.data, extent, reinterpret_cast<unsigned>(&handle));
}

Frame::Frame(unsigned _startCol, unsigned _endCol,
	     unsigned _startRow, unsigned _endRow,
	     unsigned _width, unsigned _height, unsigned _depth, 
	     const void* _input) :
  width (_endCol - _startCol),
  height(_endRow - _startRow),
  depth (_depth),
  extent(width*height*depth_bytes()) 
{
  unsigned char* data = reinterpret_cast<unsigned char*>(this+1);
  unsigned dbytes = depth_bytes();
  unsigned  w =  width*dbytes;
  unsigned _w = _width*dbytes;
  const unsigned char* src = reinterpret_cast<const unsigned char*>(_input);
  src += (_startRow * _width + _startCol) * dbytes;
  while(_startRow < _endRow) {
    memcpy(data, src, w);
    data +=  w;
    src  += _w;
  }
}

Frame::Frame(unsigned _startCol, unsigned _endCol,
	     unsigned _startRow, unsigned _endRow,
	     unsigned _width, unsigned _height, unsigned _depth, 
	     PdsLeutron::FrameHandle& _handle,
	     DmaSplice& _splice) :
  width (_endCol - _startCol),
  height(_endRow - _startRow),
  depth (_depth),
  extent(width*height*depth_bytes()) 
{
  unsigned dbytes    = depth_bytes();
  unsigned char* src = reinterpret_cast<unsigned char*>(_handle.data);
  unsigned _w = _width * dbytes;
  unsigned  w = (_endRow - _startRow) * dbytes;
  src += _startRow * _w;
  src += _startCol * dbytes;
  
  _splice.queue(src, w, (unsigned)&_handle);
  while(++_startRow < _endRow)
    _splice.queue(src += _w, w, 0);
}

Frame::Frame(const PdsLeutron::FrameHandle& handle)
{
  width  = handle.width;
  height = handle.height;
  switch(handle.format) {
  case PdsLeutron::FrameHandle::FORMAT_GRAYSCALE_8 : depth=8 ; break;
  case PdsLeutron::FrameHandle::FORMAT_GRAYSCALE_10: depth=10; break;
  case PdsLeutron::FrameHandle::FORMAT_GRAYSCALE_12: 
  default:                                           depth=12; break;
  }
  extent = width*height*((depth+7)>>3);
}
  
Frame::Frame(const Frame& f) :
  width (f.width),
  height(f.height),
  depth (f.depth),
  extent(f.extent)
{
}
