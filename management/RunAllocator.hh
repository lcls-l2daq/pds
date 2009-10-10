#ifndef Pds_RunAllocator_hh
#define Pds_RunAllocator_hh

namespace Pds {

  class RunAllocator {
  public:
    virtual unsigned alloc() {return 0;}
    virtual ~RunAllocator() {};
    enum {Error=0xffffffff};
  };

};

#endif
