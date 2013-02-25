#ifndef Pds_ObserverStreams_hh
#define Pds_ObserverStreams_hh

#include "pds/utility/WiredStreams.hh"

namespace Pds {

  class CollectionObserver;
  //class VmonAppliance;

  class ObserverStreams: public WiredStreams {
  public:

//#ifdef BUILD_LARGE_STREAM_BUFFER
//    enum { netbufdepth = 16 };
//#else
//    enum { netbufdepth = 8 };
//#endif
    enum { netbufdepth = 16 };

    enum { ebdepth     = 16 };
    enum { MaxSize     = 48*1024*1024 };

    ObserverStreams(CollectionObserver& cmgr,
                    int slowEb,
                    unsigned max_size = MaxSize);
    virtual ~ObserverStreams();
  };

}
#endif
