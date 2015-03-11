#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <byteswap.h>
#include <new>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>

#include "EvgrBoardInfo.hh"
#include "EvsManager.hh"
#include "EvrFifoServer.hh"

#include "pds/evgr/EvsMasterFIFOHandler.hh"

#include "evgr/evr/evr.hh"

#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/xtc/EvrDatagram.hh"
#include "pds/config/EvsConfigType.hh" // typedefs for the Evr config data types
#include "pds/config/CfgClientNfs.hh"

#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/OccurrenceId.hh"
#include "pds/xtc/CDatagram.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TimeStamp.hh"

using namespace Pds;

static bool _randomize_nodes = false;
static EvsMasterFIFOHandler* _fifo_handler;

static const int      giMaxEventCodes   = 64; // max number of event code configurations
static const int      giMaxPulses       = 10; 
static const int      giMaxOutputMaps   = 80; 
static const int      giMaxCalibCycles  = 500;
static EvgrBoardInfo < Evr > *erInfoGlobal; // yuck

static const unsigned running_ram    = 0;
static const unsigned persistent_ram = 1;

class EvsAction:public Action
{
public:
  EvsAction(Evr & er):_er(er)
  {
  }
  Evr & _er;
};

class EvsL1Action:public EvsAction
{
public:
  EvsL1Action(Evr & er, const Src& src, Appliance* app) :
    EvsAction(er),
    _occPool        (sizeof(Occurrence),2), 
    _app            (app),
    _outOfOrder     (false) {}

  Transition* fire(Transition* tr) 
  { 
    _outOfOrder = false;
    return _fifo_handler ? _fifo_handler->disable(tr) : tr;
  }

  InDatagram *fire(InDatagram * in)
  {
    InDatagram* out = in;
    if (_fifo_handler) {
      out = _fifo_handler->l1accept(in);
      if (!_outOfOrder && out->datagram().xtc.damage.value() & (1<<Damage::OutOfOrder)) {
        Pds::Occurrence* occ = new (&_occPool)
          Pds::Occurrence(Pds::OccurrenceId::ClearReadout);
        _app->post(occ);
        _outOfOrder = true;
      }
    }
    return out;
  }
    
private:
  GenericPool _occPool;
  Appliance*  _app;
  bool        _outOfOrder;
};

class EvsEnableAction:public EvsAction
{
public:
  EvsEnableAction(Evr&       er,
      Appliance& app) : EvsAction(er), _app(app) {}
    
  Transition* fire(Transition* tr)
  {
    if (_fifo_handler)
      _fifo_handler->enable(tr);

    return tr;
  }

  InDatagram* fire(InDatagram* tr) 
  {
    _app.post(tr);

    if (_fifo_handler)
      _fifo_handler->get_sync();

    return (InDatagram*)Appliance::DontDelete;
  }
private:
  Appliance& _app;
};

static unsigned int evrConfigSize(unsigned maxNumEventCodes, unsigned maxNumPulses, unsigned maxNumOutputMaps)
{
  return (sizeof(EvrConfigType) + 
    maxNumEventCodes * sizeof(EventCodeType) +
    maxNumPulses     * sizeof(PulseType) + 
    maxNumOutputMaps * sizeof(OutputMapType));
}

class EvsConfigManager
{
public:
  EvsConfigManager(Evr & er, CfgClientNfs & cfg, Appliance& app):
    _er (er),
    _cfg(cfg),
    _cfgtc(_evsConfigType, cfg.src()),
    _configBuffer(
      new char[ giMaxCalibCycles* evrConfigSize( giMaxEventCodes, giMaxPulses, giMaxOutputMaps ) ] ),
    _cur_config(0),
    _end_config(0),
    _occPool   (new GenericPool(sizeof(UserMessage),1)),
    _app       (app),
    _changed   (false)
  {
  }

   ~EvsConfigManager()
  {
    delete   _occPool;
    delete[] _configBuffer;
  }

  void insert(InDatagram * dg)
  {
    dg->insert(_cfgtc, _cur_config);
  }

  void configure()
  {
    if (!_changed)
      return;

    if (!_cur_config)
      return;

    _disable();

    bool slacEvr     = (reinterpret_cast<uint32_t*>(&_er)[11]&0xff) == 0x1f;
    unsigned nerror  = 0;
    UserMessage* msg = new(_occPool) UserMessage;
    msg->append(DetInfo::name(static_cast<const DetInfo&>(_cfgtc.src)));
    msg->append(":");

    const EvsConfigType& cfg = *_cur_config;

    printf("Configuring evr : %d/%d/%d\n",
           cfg.npulses(),cfg.neventcodes(),cfg.noutputs());

    _er.InternalSequenceEnable     (0);
    _er.ExternalSequenceEnable     (0);

    _er.Reset();

    // Problem in Reset() function: It doesn't reset the set and clear masks
    // workaround: manually call the clear function to set and clear all masks
    for (unsigned ram=0;ram<2;ram++) {
      for (unsigned iopcode=0;iopcode<=EVR_MAX_EVENT_CODE;iopcode++) {
        for (unsigned jSetClear=0;jSetClear<EVR_MAX_PULSES;jSetClear++)
          _er.ClearPulseMap(ram, iopcode, jSetClear, jSetClear, jSetClear);
      }
    }    
    // Another problem: The output mapping to pulses never clears
    // workaround: Set output map to the last pulse (just cleared)
    unsigned omask = 0;
    for (unsigned k = 0; k < cfg.noutputs(); k++)
      omask |= 1<<cfg.output_maps()[k].conn_id();
    omask = ~omask;

    for (unsigned k = 0; k < EVR_MAX_UNIVOUT_MAP; k++) {
      if ((omask >> k)&1 && !(k>9 && !slacEvr))
        _er.SetUnivOutMap(k, EVR_MAX_PULSES-1);
    }

    // setup map ram
    int enable = 1;

    for (unsigned k = 0; k < cfg.npulses(); k++)
      {
	const PulseType & pc = cfg.pulses()[k];
	_er.SetPulseProperties(
			       pc.pulseId(),
			       pc.polarity(),
			       1, // Enable reset from event code
			       1, // Enable set from event code
			       1, // Enable trigger from event code        
			       1  // Enable pulse
			       );
        
	unsigned prescale=pc.prescale();
	unsigned delay   =pc.delay()*prescale;
	unsigned width   =pc.width()*prescale;
	
	if (pc.pulseId()>3 && (width>>16)!=0 && !slacEvr) 
	  {
	    nerror++;
	    char buff[64];
	    sprintf(buff,"EVR pulse %d width %gs exceeds maximum.\n",
		    pc.pulseId(), double(width)/119e6);
	    msg->append(buff);
	  }
	
	_er.SetPulseParams(pc.pulseId(), prescale, delay, width);
	
	printf("pulse %d :%d %c %d/%8d/%8d\n",
	       k, pc.pulseId(), pc.polarity() ? '-':'+', 
	       prescale, delay, width);
      }
    
    _er.DumpPulses(cfg.npulses());

    for (unsigned k = 0; k < cfg.noutputs(); k++)
      {
	const OutputMapType & map = cfg.output_maps()[k];
	unsigned conn_id = map.conn_id();
	
	if (conn_id>9 && !slacEvr) {
	  nerror++;
	  char buff[64];
	  sprintf(buff,"EVR output %d out of range [0-9].\n", conn_id);
	  msg->append(buff);
	}
	
	switch (map.conn())
	  {
	  case OutputMapType::FrontPanel:
	    _er.SetFPOutMap(conn_id, Pds::EvsConfig::map(map));
	    break;
	  case OutputMapType::UnivIO:
	    _er.SetUnivOutMap(conn_id, Pds::EvsConfig::map(map));
	    break;
	  }
	
	printf("output %d : %d %x\n", k, conn_id, Pds::EvsConfig::map(map));
      }
    
    /*
     * enable event codes, and setup each event code's pulse mapping
     */
    if (cfg.neventcodes() != 1) {
      nerror++;
      char buff[64];
      sprintf(buff,"EVR neventcodes(%d) != 1 for internal sequence setup.\n", cfg.neventcodes());
      msg->append(buff);
    }
    else {
      int uEventIndex=0;

      const EvsCodeType& eventCode = cfg.eventcodes()[uEventIndex];
              
      for(unsigned ram=0; ram<2; ram++) 
	{
	  _er.SetFIFOEvent(ram, eventCode.code(), enable);
      
	  unsigned int  uPulseBit     = 0x0001;
	  uint32_t      u32Mask  = eventCode.maskTriggerP();
	  if (ram == running_ram)
	    u32Mask |= eventCode.maskTriggerR();
      
	  for ( int iPulseIndex = 0; iPulseIndex < EVR_MAX_PULSES; iPulseIndex++, uPulseBit <<= 1 )
	    {
	      if ( (u32Mask & uPulseBit) == 0 ) continue;
        
	      _er.SetPulseMap(ram, eventCode.code(), 
			      ((u32Mask & uPulseBit) != 0 ? iPulseIndex : -1 ),
			      -1, -1);
	    }
	}

      if (eventCode.period()) {
	_er.InternalSequenceSetCode    ( eventCode.code() );
	_er.InternalSequenceSetPrescale( eventCode.period()-1 );
	_er.InternalSequenceEnable     (1);
	_er.ExternalSequenceEnable     (0);
      }
      else {
	_er.ExternalSequenceSetCode    ( eventCode.code() );
	_er.ExternalSequenceEnable     (1);
	_er.InternalSequenceEnable     (0);
      }

      printf("event %d : %d period %d %x/%x group %d\n",
	     uEventIndex, eventCode.code(), eventCode.period(),
	     eventCode.maskTriggerP(),
	     eventCode.maskTriggerR(),
	     eventCode.readoutGroup());       
    }

    for(unsigned ram=0; ram<2; ram++)
      _er.DumpMapRam(ram);

    _er.MapRamEnable(persistent_ram, 1);

    if (_fifo_handler)
      _fifo_handler->set_config( &cfg );

    if (nerror) {
      _app.post(msg);
      _cfgtc.damage.increase(Damage::UserDefined);
    }
    else
      delete msg;

    _enable();

    _changed = false;
  }

  //
  //  Fetch the explicit EVR configuration and the Sequencer configuration.
  //
  void fetch(Transition* tr)
  {
    UserMessage* msg = new(_occPool) UserMessage;
    msg->append(DetInfo::name(static_cast<const DetInfo&>(_cfgtc.src)));
    msg->append(":");

    _cfgtc.damage = 0;
    _cfgtc.damage.increase(Damage::UserDefined);

    int len = _cfg.fetch(*tr, _evsConfigType, _configBuffer);
    if (len <= 0)
    {
      _cur_config = 0;
      _cfgtc.extent = sizeof(Xtc);
      _cfgtc.damage.increase(Damage::UserDefined);
      printf("Config::configure failed to retrieve Evr configuration\n");
      msg->append("failed to read Evr configuration\n");
      _app.post(msg);
      return;
    }

    _cur_config = reinterpret_cast<const EvsConfigType*>(_configBuffer);
    _end_config = _configBuffer+len;
    _cfgtc.extent = sizeof(Xtc) + Pds::EvsConfig::size(*_cur_config);

    delete msg;
    _cfgtc.damage = 0;

    _changed = true;
  }

  void advance()
  {
    const EvsConfigType* cur_config(_cur_config);

    int len = Pds::EvsConfig::size(*_cur_config);
    const char* nxt_config = reinterpret_cast<const char*>(_cur_config)+len;
    if (nxt_config < _end_config)
      _cur_config = reinterpret_cast<const EvsConfigType*>(nxt_config);
    else
      _cur_config = reinterpret_cast<const EvsConfigType*>(_configBuffer);
    _cfgtc.extent = sizeof(Xtc) + Pds::EvsConfig::size(*_cur_config);

    _changed = (_cur_config != cur_config);
  }

private:
  void _enable()
  {
    //    _er.IrqEnable(EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_EVENT);
    //    _er.EnableFIFO(1);
    _er.Enable(1);
  }

  void _disable()
  {
    //    _er.IrqEnable(0);
    _er.Enable(0);
    //    _er.EnableFIFO(0);
  }
  
private:
  Evr&            _er;
  CfgClientNfs&   _cfg;
  Xtc             _cfgtc;
  char *          _configBuffer;
  const EvsConfigType* _cur_config;
  const char*          _end_config;
  GenericPool*         _occPool;
  Appliance&           _app;
  bool                 _changed;
};

class EvsConfigAction: public Action {
public:
  EvsConfigAction(EvsConfigManager& cfg) : _cfg(cfg) {}
public:
  Transition* fire(Transition* tr) 
  { 
    _cfg.fetch(tr);
    _cfg.configure(); // added by Tomy

    if (_fifo_handler)
      _fifo_handler->config(tr);

    return tr;
  }
  InDatagram* fire(InDatagram* dg) { 
    _cfg.insert(dg); 
    return dg; 
  }
private:
  EvsConfigManager& _cfg;
};

class EvsBeginCalibAction: public Action {
public:
  EvsBeginCalibAction(EvsConfigManager& cfg) : _cfg(cfg) {}
public:
  Transition* fire(Transition* tr) { 
    _cfg.configure(); 
    return tr; }
  InDatagram* fire(InDatagram* dg) { _cfg.insert(dg); return dg; }
private:
  EvsConfigManager& _cfg;
};

class EvsEndCalibAction: public Action
{
public:
  EvsEndCalibAction(EvsConfigManager& cfg): _cfg(cfg) {}
public:
  Transition *fire(Transition * tr) { 
    if (_fifo_handler)
      _fifo_handler->endcalib(tr);
    return tr; 
  }
  InDatagram *fire(InDatagram * dg) { _cfg.advance(); return dg; }
private:
  EvsConfigManager& _cfg;
};


class EvsAllocAction:public Action
{
public:
  EvsAllocAction(CfgClientNfs& cfg, 
                 Evr&          er,
                 Appliance&         app,
                 EvsConfigManager&  cmgr,
                 EvrFifoServer&     srv) :
    _cfg(cfg), _er(er), _app(app), _cmgr(cmgr), _srv(srv),
    _task     (new Task(TaskObject("evrsync"))),
    _sync_task(new Task(TaskObject("slvsync")))
  {
    _fifo_handler = 0;
  }
  Transition *fire(Transition * tr)
  {
    const Allocate & alloc = reinterpret_cast < const Allocate & >(*tr);
    _cfg.initialize(alloc.allocation());

    unsigned nnodes = alloc.allocation().nnodes();
    
    // find max value of segment group id
    unsigned iMaxGroup  = 0;
    for (unsigned n = 0; n < nnodes; n++)
    {
      const Node *node = alloc.allocation().node(n);
      if (node->level() == Level::Segment) 
      {
        if (node->group() > iMaxGroup)
          iMaxGroup = node->group();
      }
    }

    _fifo_handler = new EvsMasterFIFOHandler(
					     _er,
					     _cfg.src(),
					     _app,
					     _srv,
					     alloc.allocation().partitionid(),
					     iMaxGroup,
					     alloc.allocation().nnodes(Level::Event),
					     _randomize_nodes,
					     _task);
    return tr;
  }
private:
  CfgClientNfs& _cfg;
  Evr&          _er;
  Appliance&    _app;
  EvsConfigManager& _cmgr;
  EvrFifoServer& _srv;
  Task*         _task;
  Task*         _sync_task;
};

class EvsShutdownAction:public Action
{
public:
  EvsShutdownAction() {}
  Transition *fire(Transition * tr) {
    if (_fifo_handler) {
      delete _fifo_handler;
      _fifo_handler = 0;
    }
    return tr;
  }
};


extern "C"
{
  void evsmgr_sig_handler(int parm)
  {
    Evr & er = erInfoGlobal->board();
    Pds::FIFOEvent fe;
    while( ! er.GetFIFOEvent(&fe) ) {
      if (_fifo_handler)
        _fifo_handler->fifo_event(fe);
    }
    int fdEr = erInfoGlobal->filedes();
    er.IrqHandled(fdEr);
  }
}

Appliance & EvsManager::appliance()
{
  return _fsm;
}

Server& EvsManager::server()
{
  return *_server;
}

EvsManager::EvsManager(EvgrBoardInfo < Evr > &erInfo, CfgClientNfs & cfg) :
  _er(erInfo.board()), _fsm(*new Fsm),
  _server(new EvrFifoServer(cfg.src()))
{
  EvsConfigManager* cmgr = new EvsConfigManager(_er, cfg, _fsm);

  _fsm.callback(TransitionId::Map            , new EvsAllocAction     (cfg,_er,_fsm, *cmgr, *_server));
  _fsm.callback(TransitionId::Unmap          , new EvsShutdownAction);
  _fsm.callback(TransitionId::Configure      , new EvsConfigAction    (*cmgr));
  _fsm.callback(TransitionId::BeginCalibCycle, new EvsBeginCalibAction(*cmgr));
  _fsm.callback(TransitionId::EndCalibCycle  , new EvsEndCalibAction  (*cmgr));
  _fsm.callback(TransitionId::Enable         , new EvsEnableAction    (_er,_fsm));
  _fsm.callback(TransitionId::L1Accept       , new EvsL1Action(_er, cfg.src(), &_fsm));

  _er.IrqAssignHandler(erInfo.filedes(), &evsmgr_sig_handler);
  erInfoGlobal = &erInfo;

  // Unix signal support
  struct sigaction int_action;

  int_action.sa_handler = EvsManager::sigintHandler;
  sigemptyset(&int_action.sa_mask);
  int_action.sa_flags = 0;
  int_action.sa_flags |= SA_RESTART;

  if (sigaction(SIGINT, &int_action, 0) > 0)
    printf("Couldn't set up SIGINT handler\n");
  if (sigaction(SIGKILL, &int_action, 0) > 0)
    printf("Couldn't set up SIGKILL handler\n");
  if (sigaction(SIGSEGV, &int_action, 0) > 0)
    printf("Couldn't set up SIGSEGV handler\n");
  if (sigaction(SIGABRT, &int_action, 0) > 0)
    printf("Couldn't set up SIGABRT handler\n");
  if (sigaction(SIGTERM, &int_action, 0) > 0)
    printf("Couldn't set up SIGTERM handler\n");
    
}

EvsManager::~EvsManager()
{
}

void EvsManager::sigintHandler(int iSignal)
{
  printf("signal %d received\n", iSignal);

  if (erInfoGlobal && erInfoGlobal->mapped())
  {
    Evr & er = erInfoGlobal->board();
    printf("Stopping triggers and multicast... ");
    er.MapRamEnable(persistent_ram,1);
    printf("stopped\n");
  }
  else
  {
    printf("evr not mapped\n");
  }
  exit(0);
}

void EvsManager::randomize_nodes(bool v) { _randomize_nodes=v; }

