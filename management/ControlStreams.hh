#ifndef PDS_CONTROLSTREAMS_HH
#define PDS_CONTROLSTREAMS_HH

#include "pds/utility/WiredStreams.hh"

namespace Pds {
  
  class PartitionMember;
  //class ControlOccHandler;

  class ControlStreams: public WiredStreams {
  public:
    enum { netbufdepth = 32 };
    
    /*
     * Buffer size to hold the transition event
     *
     * Note: For storing the make-up event of princeton program,
     *   the buffer size need to be large than 2048*2048*2 + sizeof(Princeton::FrameV1)
     */
    enum { MaxSize = 8*1024*1024 + 512 }; 
  public:
    ControlStreams(PartitionMember&);
    virtual ~ControlStreams();
    
  private:
    //    ControlOccHandler* _occ_handler;
  };
}

#endif
