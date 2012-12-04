#ifndef Pds_Frame_hh
#define Pds_Frame_hh

#include "pds/camera/FrameType.hh"

#include <new>

namespace Pds {

  class FrameServerMsg;

  class Frame : public FrameType {
  public:
    Frame() {}
    //  Frame with unassigned contents
    Frame(const FrameServerMsg&);
    //  Copy frame contents from "input"
    Frame(const FrameServerMsg&,
	  const void* input);
    //  Copy frame contents from subsection of "input"
    Frame(unsigned startCol, unsigned endCol,
	  unsigned startRow, unsigned endRow,
	  const FrameServerMsg&);
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
