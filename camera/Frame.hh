#ifndef Pds_Frame_hh
#define Pds_Frame_hh

namespace PdsLeutron {
  class FrameHandle;
};

namespace Pds {

  class DmaSplice;

  class Frame {
  public:
    Frame() {}
    //  Frame with unassigned contents
    Frame(unsigned _width, unsigned _height, unsigned _depth);
    //  Copy frame contents from "input"
    Frame(unsigned _width, unsigned _height, unsigned _depth, 
	  const void* input);
    Frame(unsigned _width, unsigned _height, unsigned _depth, 
	  PdsLeutron::FrameHandle&,
	  DmaSplice&  splice);
    //  Copy frame contents from subsection of "input"
    Frame(unsigned _startCol, unsigned _endCol,
	  unsigned _startRow, unsigned _endRow,
	  unsigned _width, unsigned _height, unsigned _depth, 
	  const void* input);
    Frame(unsigned _startCol, unsigned _endCol,
	  unsigned _startRow, unsigned _endRow,
	  unsigned _width, unsigned _height, unsigned _depth, 
	  PdsLeutron::FrameHandle&,
	  DmaSplice&  splice);
    //  Copy frame from grabber (does not copy contents)
    Frame(const PdsLeutron::FrameHandle&);
    //  Copy constructor (does not copy contents)
    Frame(const Frame&);

    unsigned long width;
    unsigned long height;
    unsigned long depth;
    unsigned long extent;

    unsigned depth_bytes() const;
    const unsigned char* data() const;
    const unsigned char* pixel(unsigned x,unsigned y) const;
  };

};

inline unsigned Pds::Frame::depth_bytes() const
{
  return (depth+7)>>3;
}

inline const unsigned char* Pds::Frame::data() const
{
  return reinterpret_cast<const unsigned char*>(this+1);
}

inline const unsigned char* Pds::Frame::pixel(unsigned x,
					      unsigned y) const
{
  return data()+(y*width+x)*depth_bytes();
}

#endif
