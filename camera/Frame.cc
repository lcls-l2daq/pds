#include "pds/camera/Frame.hh"

#include "pds/camera/FrameServerMsg.hh"
#include "pdsdata/xtc/Xtc.hh"

#include <string.h>
#include <stdio.h>

using namespace Pds;

Frame::Frame(const FrameServerMsg& msg) :
  FrameType(msg.width, msg.height, msg.depth, msg.offset)
{
}

Frame::Frame(const FrameServerMsg& msg,
	     const void* input) :
  FrameType(msg.width, msg.height, msg.depth, msg.offset)
{
  switch (msg.intlv) {
  case FrameServerMsg::MidTopLine:
    {
      long long w = msg.width*depth_bytes();
      const uint8_t* s1 = (const uint8_t*)input;
      const uint8_t* s2 = s1 + w;
      uint8_t* d1 = data() + int(w)*(int(msg.height)/2-1);
      uint8_t* d2 = d1 + w;
      for(unsigned i=0; i<msg.height/2; i++) {
	memcpy(d1, s1, w);
	memcpy(d2, s2, w);
	d1 -= w;
	d2 += w;
	s1 += w*2;
	s2 += w*2;
      }
      break;
    }
  case FrameServerMsg::None:
  default:
    memcpy(data(), input, data_size());
    break;
  }
}

Frame::Frame(unsigned startCol, unsigned endCol,
	     unsigned startRow, unsigned endRow,
	     const FrameServerMsg& msg) :
  FrameType( endCol - startCol, endRow - startRow,
	     msg.depth, msg.offset )
{
  unsigned dbytes = depth_bytes();
  unsigned  w = dbytes*width();
  unsigned fw = dbytes*msg.width;
  unsigned rw = w < fw ? w : fw;

  switch (msg.intlv) {
  case FrameServerMsg::MidTopLine:
    { const uint8_t* s1 = (const uint8_t*)msg.data + startCol*dbytes;
      const uint8_t* s2 = s1 + fw;
      int dh = int(msg.height)/2-1-int(startRow);
      uint8_t* d1 = data() + int(w)*dh;
      uint8_t* d2 = d1 + w;
      uint8_t* const d_start = data();
      uint8_t* const d_end   = d_start + data_size();

      for(unsigned i=0; i<msg.height/2; i++) {
	if (d1>=d_start && d1<d_end)
	  memcpy(d1, s1, rw);
	if (d2>=d_start && d2<d_end)
	  memcpy(d2, s2, rw);
	d1 -= w;
	d2 += w;
	s1 += fw*2;
	s2 += fw*2;
      }
    }
    break;
  default:
    { const unsigned char* src = reinterpret_cast<const unsigned char*>(msg.data);
      unsigned char* dst = data();
      src += (startRow * msg.width + startCol) * dbytes;
      while(startRow++ < endRow) {
	memcpy(dst, src, w);
	dst +=  w;
	src += fw;
      }
    }
    break;
  }
}

Frame::Frame(const Frame& f) :
  FrameType( f.width(), f.height(),
	     f.depth(), f.offset() )
{
}

