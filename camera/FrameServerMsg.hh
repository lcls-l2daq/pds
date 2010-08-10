#ifndef Pds_FrameServerMsg_hh
#define Pds_FrameServerMsg_hh

#include "pds/service/LinkedList.hh"
#include "pdsdata/xtc/Damage.hh"
#include "pdsdata/xtc/ClockTime.hh"

namespace PdsLeutron {
  class FrameHandle;
};

namespace Pds {

  class FrameServerMsg : public LinkedList<FrameServerMsg> {
  public:
    enum Type { NewFrame, Fragment };

    FrameServerMsg(Type _type,
		   PdsLeutron::FrameHandle* _handle,
		   unsigned _count,
		   unsigned _offset,
		   unsigned _seconds,
		   unsigned _nseconds) :
      type  (_type),
      handle(_handle),
      count (_count),
      offset(_offset),
      time  (_seconds,_nseconds),
      damage(0) {}
    
    Type type;
    PdsLeutron::FrameHandle* handle;
    unsigned count;
    unsigned offset;
    unsigned extent;
    ClockTime time;    
    Damage    damage;
  };
  
};

#endif
