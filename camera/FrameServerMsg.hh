#ifndef Pds_FrameServerMsg_hh
#define Pds_FrameServerMsg_hh

#include "pds/service/LinkedList.hh"
#include "pdsdata/xtc/Damage.hh"

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
		   unsigned _offset) :
      type  (_type),
      handle(_handle),
      count (_count),
      offset(_offset),
      damage(0) {}
    
    Type type;
    PdsLeutron::FrameHandle* handle;
    unsigned count;
    unsigned offset;
    unsigned extent;
    Damage   damage;
  };
  
};

#endif
