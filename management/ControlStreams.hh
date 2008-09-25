#ifndef PDS_CONTROLSTREAMS_HH
#define PDS_CONTROLSTREAMS_HH

#include "pds/utility/WiredStreams.hh"

namespace Pds {
  
  class CollectionManager;
  //class ControlOccHandler;

  class ControlStreams: public WiredStreams {
  public:
    enum { netbufdepth = 32 };
  public:
    ControlStreams(CollectionManager&);
    virtual ~ControlStreams();
    
  private:
    //    ControlOccHandler* _occ_handler;
  };
}

#endif
