#ifndef ODFEVENTSTREAMS_HH
#define ODFEVENTSTREAMS_HH

#include "pds/utility/WiredStreams.hh"

namespace Pds {

  class PartitionMember;
  //class VmonAppliance;

  class EventStreams: public WiredStreams {
  public:
    enum { netbufdepth = 8 };
    enum { ebdepth     = 16 };
    enum { MaxSize     = 4*1024*1024 };

    EventStreams(PartitionMember& cmgr);
    virtual ~EventStreams();

  private:
    //  VmonAppliance* _vmom_appliance;
  };

}
#endif
