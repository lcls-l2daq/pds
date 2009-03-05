#include "pds/camera/Frame.hh"
#include "pds/camera/DmaSplice.hh"
#include "pds/camera/FrameHandle.hh"

#include "pdsdata/xtc/Xtc.hh"

#include <string.h>

using namespace Pds;

Frame::Frame(unsigned width, unsigned height, 
	     unsigned depth, unsigned offset) :
  Camera::FrameV1(width, height, depth, offset)
{
}

Frame::Frame(unsigned width, unsigned height, 
	     unsigned depth, unsigned offset,
	     const void* input) :
  Camera::FrameV1(width, height, depth, offset)
{
  memcpy(data(), input, data_size());
}

Frame::Frame(unsigned width, unsigned height, 
	     unsigned depth, unsigned offset,
	     PdsLeutron::FrameHandle& handle,
	     DmaSplice&  splice) :
  Camera::FrameV1(width, height, depth, offset)
{
  splice.queue(handle.data, data_size(), reinterpret_cast<unsigned>(&handle));
}

Frame::Frame(unsigned startCol, unsigned endCol,
	     unsigned startRow, unsigned endRow,
	     unsigned fwidth, unsigned fheight, 
	     unsigned depth , unsigned offset,
	     const void* input) :
  Camera::FrameV1( endCol - startCol, endRow - startRow,
		   depth, offset )
{
  unsigned dbytes = depth_bytes();
  unsigned  w = dbytes*width();
  unsigned fw = dbytes*fwidth;
  unsigned char* dst = data();
  const unsigned char* src = reinterpret_cast<const unsigned char*>(input);
  src += (startRow * fwidth + startCol) * dbytes;
  while(startRow < endRow) {
    memcpy(dst, src, w);
    dst +=  w;
    src += fw;
  }
}

Frame::Frame(unsigned startCol, unsigned endCol,
	     unsigned startRow, unsigned endRow,
	     unsigned fwidth, unsigned fheight, 
	     unsigned  depth, unsigned offset,
	     PdsLeutron::FrameHandle& handle,
	     DmaSplice& splice) :
  Camera::FrameV1( endCol - startCol, endRow - startRow,
		   depth, offset )
{
  unsigned dbytes    = depth_bytes();
  unsigned  w = dbytes*width();
  unsigned fw = dbytes*fwidth;
  unsigned char* src = reinterpret_cast<unsigned char*>(handle.data);
  src += startRow * fw;
  src += startCol * dbytes;
  
  splice.queue(src, w, (unsigned)&handle);
  while(++startRow < endRow)
    splice.queue(src += fw, w, 0);
}

Frame::Frame(const PdsLeutron::FrameHandle& handle,
	     unsigned offset) :
  Camera::FrameV1( handle.width, handle.height, handle.depth(), offset )
{
}
  
Frame::Frame(const Frame& f) :
  Camera::FrameV1( f.width(), f.height(),
		   f.depth(), f.offset() )
{
}

