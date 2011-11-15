#include "pds/camera/Frame.hh"

#include "pdsdata/xtc/Xtc.hh"

#include <string.h>

using namespace Pds;

Frame::Frame(unsigned width, unsigned height, 
	     unsigned depth, unsigned offset) :
  FrameType(width, height, depth, offset)
{
}

Frame::Frame(unsigned width, unsigned height, 
	     unsigned depth, unsigned offset,
	     const void* input) :
  FrameType(width, height, depth, offset)
{
  memcpy(data(), input, data_size());
}

Frame::Frame(unsigned startCol, unsigned endCol,
	     unsigned startRow, unsigned endRow,
	     unsigned fwidth, unsigned fheight, 
	     unsigned depth , unsigned offset,
	     const void* input) :
  FrameType( endCol - startCol, endRow - startRow,
		   depth, offset )
{
  unsigned dbytes = depth_bytes();
  unsigned  w = dbytes*width();
  unsigned fw = dbytes*fwidth;
  unsigned char* dst = data();
  const unsigned char* src = reinterpret_cast<const unsigned char*>(input);
  src += (startRow * fwidth + startCol) * dbytes;
  while(startRow++ < endRow) {
    memcpy(dst, src, w);
    dst +=  w;
    src += fw;
  }
}

Frame::Frame(const Frame& f) :
  FrameType( f.width(), f.height(),
		   f.depth(), f.offset() )
{
}

