#ifndef Pds_Sequencer_hh
#define Pds_Sequencer_hh

namespace Pds {

  class Sequencer {
  public:
    virtual ~Sequencer() {}
    virtual void start() = 0;
    virtual void stop () = 0;
  };

};

#endif
