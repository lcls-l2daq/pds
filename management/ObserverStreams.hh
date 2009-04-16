#ifndef Pds_ObserverStreams_hh
#define Pds_ObserverStreams_hh

#include "pds/utility/WiredStreams.hh"

namespace Pds {

  class CollectionObserver;
  //class VmonAppliance;

  class ObserverStreams: public WiredStreams {
  public:
    enum { netbufdepth = 8 };
    enum { ebdepth     = 16 };
    enum { MaxSize     = 4*1024*1024 };

    ObserverStreams(CollectionObserver& cmgr);
    virtual ~ObserverStreams();
  };

}
#endif
