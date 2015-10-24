#include "pds/evgr/EvsMasterFIFOHandler.hh"

#include "pds/utility/Appliance.hh"
#include "pds/utility/Occurrence.hh"

static const int      running_ram    =  0;
static const int      persistent_ram =  1;

using namespace Pds;

EvsMasterFIFOHandler::EvsMasterFIFOHandler(Evr&       er,
                                           const Src& src,
                                           Appliance& app,
                                           EvrFifoServer& srv,
                                           unsigned   partition,
                                           int        iMaxGroup,
                                           unsigned   neventnodes,
                                           bool       randomize,
                                           Task*      task,
                                           VmonEvr&   vmon):
  MasterFIFOHandler(er,src,app,srv,partition,iMaxGroup,0,neventnodes,randomize,task,vmon),
  _pool               (sizeof(Occurrence),1)
{
}

EvsMasterFIFOHandler::~EvsMasterFIFOHandler()
{
}

void EvsMasterFIFOHandler::fifo_event(const FIFOEvent& fe)
{
  if (enabled(fe)) {
    /*
     * Determine if we need to start L1Accept
     *
     * Rule: If we have got an readout event before, and get the terminator event now,
     *    then we will start L1Accept
     */

    _state.update(fe);

    if (_state.uMaskReadout != 0 || _state.ncommands)
      {
  startL1Accept(fe, false);
      }
  }
}

void EvsMasterFIFOHandler::get_sync()
{
  //  Empty FIFO
  FIFOEvent tfe;
  while( !_er.GetFIFOEvent(&tfe) )
    ;

  //  Enable Triggers
  _er.IrqEnable(EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_EVENT);
  _er.EnableFIFO(1);
  _er.MapRamEnable(running_ram, 1);

  //  Generate EVR Enabled Occurrence
  _app.post(new (&_pool) Occurrence(OccurrenceId::EvrEnabled));
}

Transition* EvsMasterFIFOHandler::disable     (Transition* tr)
{
  //  Disable Triggers
  _er.MapRamEnable(persistent_ram, 1);
  _er.IrqEnable(0);
  _er.EnableFIFO(0);

  release_sync();

  return tr;
}

void        EvsMasterFIFOHandler::set_config  (const EvsConfigType* pEvrConfig)
{
  _state.configure( pEvrConfig->eventcodes() );
  if (pEvrConfig->neventcodes()>0 &&
      pEvrConfig->eventcodes()[0].period()<(119000000/360))
    validateFiducial(false);
}

