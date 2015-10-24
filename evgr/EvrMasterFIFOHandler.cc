#include "pds/evgr/EvrMasterFIFOHandler.hh"
#include "pds/utility/Appliance.hh"

#include <stdio.h>

using namespace Pds;

EvrMasterFIFOHandler::EvrMasterFIFOHandler(Evr&       er,
                                           const Src& src,
                                           Appliance& app,
                                           EvrFifoServer& srv,
                                           unsigned   partition,
                                           int        iMaxGroup,
                                           unsigned   module,
                                           unsigned   neventnodes,
                                           bool       randomize,
                                           Task*      task,
                                           VmonEvr&   vmon):
  MasterFIFOHandler(er,src,app,srv,partition,iMaxGroup,module,neventnodes,randomize,task,vmon),
  _sync            (*this, er, partition, task, _outlet, app),
  _tr              (0)
{
}

EvrMasterFIFOHandler::~EvrMasterFIFOHandler()
{
}

void EvrMasterFIFOHandler::fifo_event(const FIFOEvent& fe)
{
  if ( _sync.handle(fe) )
    return;

  if (enabled(fe)) {
    /*
     * Determine if we need to start L1Accept
     *
     * Rule: If we have got an readout event before, and get the terminator event now,
     *    then we will start L1Accept
     */
    if (fe.EventCode == TERMINATOR) {
      if (_state.uMaskReadout != 0 || _state.ncommands)
        startL1Accept(fe, false);
      return;
    }

    _state.update( fe );
  }
}

void EvrMasterFIFOHandler::get_sync()
{
  timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 20000000;
  nanosleep(&ts,0);

  _sync.initialize(true);
}

void EvrMasterFIFOHandler::release_sync()
{
  MasterFIFOHandler::release_sync();
  _app.post(_tr);
}

Transition* EvrMasterFIFOHandler::disable     (Transition* tr)
{
  _tr = tr;
  _sync.initialize(false);
  return (Transition*)Appliance::DontDelete;
}
