#ifndef Pds_Frame_hh
#define Pds_Frame_hh

#include "pds/camera/FrameType.hh"

#include <new>

namespace PdsLeutron {
  class FrameHandle;
};

namespace Pds {

  class DmaSplice;

  class Frame : public FrameType {
  public:
    Frame() {}
    //  Frame with unassigned contents
    Frame(unsigned width, unsigned height, 
	  unsigned depth, unsigned offset);
    //  Copy frame contents from "input"
    Frame(unsigned width, unsigned height, 
	  unsigned depth, unsigned offset,
	  const void* input);
    //  Queue the frame's DMA buffer to "splice"
    Frame(unsigned width, unsigned height, 
	  unsigned depth, unsigned offset,
	  PdsLeutron::FrameHandle&,
	  DmaSplice&  splice);
    //  Copy frame contents from subsection of "input"
    Frame(unsigned startCol, unsigned endCol,
	  unsigned startRow, unsigned endRow,
	  unsigned width, unsigned height, 
	  unsigned depth, unsigned offset,
	  const void* input);
    //  Queue the frame's ROI to "splice"
    Frame(unsigned startCol, unsigned endCol,
	  unsigned startRow, unsigned endRow,
	  unsigned width, unsigned height, 
	  unsigned depth, unsigned offset,
	  PdsLeutron::FrameHandle&,
	  DmaSplice&  splice);
    //  Copy frame header from grabber (does not copy contents)
    Frame(const PdsLeutron::FrameHandle&,
	  unsigned offset);
    //  Copy constructor (does not copy contents)
    Frame(const Frame&);
  public:
    unsigned char* data();
  };

  inline unsigned char* Frame::data()
  {
    return const_cast<unsigned char*>(FrameType::data());
  }
};

#endif
