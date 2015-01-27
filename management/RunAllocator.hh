#ifndef Pds_RunAllocator_hh
#define Pds_RunAllocator_hh

#include <vector>
#include <string>

namespace Pds {

  class RunAllocator {
  public:
    virtual unsigned alloc() {return 0;}
    virtual int reportOpenFile(int, int, int, int, std::string&, std::string&) {return 0;}
    virtual int reportDetectors(int, int, std::vector<std::string>&) {return 0;}
    virtual int reportTotals(int, int, long, long, double) {return 0;};

    virtual ~RunAllocator() {};
    enum {Error=0xffffffff};
  };

};

#endif
