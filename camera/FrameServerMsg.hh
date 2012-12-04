#ifndef Pds_FrameServerMsg_hh
#define Pds_FrameServerMsg_hh

#include "pds/service/LinkedList.hh"
#include "pdsdata/xtc/Damage.hh"

namespace Pds {

  class FrameServerMsg : public LinkedList<FrameServerMsg> {
  public:
    enum Type { NewFrame, Fragment };
    enum Intlv { None, MidTopLine };

    FrameServerMsg(Type     _type,
                   void*    _data,
                   unsigned _width,
                   unsigned _height,
                   unsigned _depth,
		   unsigned _count,
		   unsigned _offset,
                   void(*_release)(void*)=0,
                   void*    _arg=0) :
      type  (_type),
      data  (_data),
      width (_width),
      height(_height),
      depth (_depth),
      intlv (None),
      count (_count),
      offset(_offset),
      damage(0),
      release(_release),
      arg    (_arg) {}
    
    ~FrameServerMsg() { if (release!=0) release(arg); }
    Type     type;
    void*    data;
    unsigned width;
    unsigned height;
    unsigned depth;
    Intlv    intlv;
    unsigned count;
    unsigned offset;
    unsigned extent;
    Damage   damage;
    void   (*release)(void*);
    void    *arg;
  };
  
};

#endif
