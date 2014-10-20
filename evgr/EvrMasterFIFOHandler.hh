#ifndef Pds_EvrMasterFIFOHandler_hh
#define Pds_EvrMasterFIFOHandler_hh

/**
 **  class EvrMasterFIFOHandler - a MasterFIFOHandler implementation that knows
 **    the eventcode stream is complete when the TERMINATOR eventcode is seen.
 **    This class also initiates synchronization with the slave EVRs on enable
 **    and disable.
 **/

#include "pds/evgr/MasterFIFOHandler.hh"

#include "pds/evgr/EvrSync.hh"

namespace Pds {

  class EvrMasterFIFOHandler : public MasterFIFOHandler {
  public:
    enum { TERMINATOR         = 1 };
  public:
    EvrMasterFIFOHandler(Evr&,
       const Src&,
       Appliance&,
                         EvrFifoServer&,
       unsigned partition,
       int      iMaxGroup,
       unsigned module,
       unsigned neventnodes,
       bool     randomize,
       Task*    task);
    virtual ~EvrMasterFIFOHandler();
  public:
    virtual void        fifo_event  (const FIFOEvent&);  // formerly 'xmit'
    virtual Transition* disable     (Transition*);
    virtual void        get_sync    ();
    virtual void        release_sync();
  private:
    EvrSyncMaster         _sync;
    Transition*           _tr;
  };
};

#endif
