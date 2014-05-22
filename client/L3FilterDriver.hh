#ifndef Pds_L3FilterDriver_hh
#define Pds_L3FilterDriver_hh

#include "pds/utility/Appliance.hh"
#include "pdsdata/app/L3FilterModule.hh"
#include "pdsdata/xtc/XtcIterator.hh"

namespace Pds {
  class L3FilterModule;
  class L3FilterDriver : public Appliance,
                         public XtcIterator {
  public:
    L3FilterDriver(L3FilterModule*, bool lveto);
    ~L3FilterDriver();
  public:
    Transition* transitions(Transition*);
    InDatagram* events     (InDatagram*);
  public:
    int  process(Xtc*);
  private:
    L3FilterModule* _m;
    bool            _lVeto;
    bool            _event;
  };
};
    
#endif
