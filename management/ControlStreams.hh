#ifndef PDS_CONTROLSTREAMS_HH
#define PDS_CONTROLSTREAMS_HH

#include "pds/utility/WiredStreams.hh"

namespace Pds {
  
  class PartitionMember;
  //class ControlOccHandler;

  class ControlStreams: public WiredStreams {
  public:
    enum { netbufdepth = 32 };
    enum { MaxSize = 1024*1024 };
  public:
    ControlStreams(PartitionMember&);
    virtual ~ControlStreams();
    
  private:
    //    ControlOccHandler* _occ_handler;
  };
}

#endif
