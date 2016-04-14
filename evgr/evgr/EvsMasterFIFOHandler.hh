#ifndef Pds_EvsMasterFIFOHandler_hh
#define Pds_EvsMasterFIFOHandler_hh

/**
 **  class EvsMasterFIFOHandler - a MasterFIFOHandler implementation that knows
 **    the eventcode stream only contains one eventcode.  Synchronization on
 **    enable/disable is unnecessary (no slaves).
 **/

#include "pds/evgr/MasterFIFOHandler.hh"

#include "pds/config/EvsConfigType.hh"
#include "pds/service/GenericPool.hh"

namespace Pds {

  class EvsMasterFIFOHandler : public MasterFIFOHandler {
  public:
    EvsMasterFIFOHandler(Evr&, 
			 const Src&, 
			 Appliance&, 
			 EvrFifoServer&,
			 unsigned partition,
			 int      iMaxGroup,
			 unsigned neventnodes,
			 bool     randomize,
			 Task*    task,
                         VmonEvr& vmon);
    virtual ~EvsMasterFIFOHandler();
  public:
    virtual void        fifo_event  (const FIFOEvent&);  // formerly 'xmit'
    virtual Transition* disable     (Transition*);
    virtual void        set_config  (const EvrConfigType*) {}
    virtual void        set_config  (const EvsConfigType*);
    virtual void        get_sync    ();
    virtual void        release_sync() {}

  private:
    GenericPool           _pool;
  };
};

#endif
