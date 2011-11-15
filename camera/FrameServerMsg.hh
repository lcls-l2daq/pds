#ifndef Pds_FrameServerMsg_hh
#define Pds_FrameServerMsg_hh

#include "pds/service/LinkedList.hh"
#include "pdsdata/xtc/Damage.hh"

namespace Pds {

  class FrameServerMsg : public LinkedList<FrameServerMsg> {
  public:
    enum Type { NewFrame, Fragment };

    FrameServerMsg(Type     _type,
                   void*    _data,
                   unsigned _width,
                   unsigned _height,
                   unsigned _depth,
		   unsigned _count,
		   unsigned _offset) :
      type  (_type),
      data  (_data),
      width (_width),
      height(_height),
      depth (_depth),
      count (_count),
      offset(_offset),
      damage(0) {}
    
    Type     type;
    void*    data;
    unsigned width;
    unsigned height;
    unsigned depth;
    unsigned count;
    unsigned offset;
    unsigned extent;
    Damage   damage;
  };
  
};

#endif
