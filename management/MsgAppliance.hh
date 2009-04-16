#ifndef Pds_MsgAppliance_hh
#define Pds_MsgAppliance_hh

#include "pds/utility/Appliance.hh"

namespace Pds {
  class MsgAppliance : public Appliance {
  public:
    MsgAppliance();
    ~MsgAppliance();
  public:
    virtual Transition* transitions(Transition*);
    virtual InDatagram* occurrences(InDatagram*);
    virtual InDatagram* events     (InDatagram*);
  private:
    int _fd;
  };
};

#endif
