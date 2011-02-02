#ifndef Pds_ObserverStreams_hh
#define Pds_ObserverStreams_hh

#include "pds/utility/WiredStreams.hh"

namespace Pds {

  class CollectionObserver;
  //class VmonAppliance;

  class ObserverStreams: public WiredStreams {
  public:

#ifdef BUILD_LARGE_STREAM_BUFFER
    enum { netbufdepth = 16 };
#else
    enum { netbufdepth = 8 };
#endif

    enum { ebdepth     = 16 };
    enum { MaxSize     = 16*1024*1024 };

    ObserverStreams(CollectionObserver& cmgr);
    virtual ~ObserverStreams();
  };

}
#endif
