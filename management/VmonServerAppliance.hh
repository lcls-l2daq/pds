#ifndef Pds_VmonServerAppliance_hh
#define Pds_VmonServerAppliance_hh

#include "pds/utility/Appliance.hh"

#include "pdsdata/xtc/Src.hh"
#include "pds/mon/MonCds.hh"
#include "pds/mon/MonUsage.hh"

namespace Pds {

  class MonServer;

  class VmonServerAppliance : public Appliance {
  public:
    VmonServerAppliance(const Src&);
    ~VmonServerAppliance();
  public:
    Transition* transitions(Transition*);
    InDatagram* occurrences(InDatagram*);
    InDatagram* events     (InDatagram*);
  private:
    Src        _src;
    int        _partition;
  };

};

#endif
    
