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
#include "EvrManager.hh"
#include "EvrCfgClient.hh"

#include "pds/evgr/EvrMasterFIFOHandler.hh"
#include "pds/evgr/EvrSlaveFIFOHandler.hh"

#include "evgr/evr/evr.hh"

#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/xtc/EvrDatagram.hh"
#include "pds/config/EvrConfigType.hh" // typedefs for the Evr config data types

#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/OccurrenceId.hh"
#include "pds/xtc/CDatagram.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TimeStamp.hh"

using namespace Pds;

static EvrFIFOHandler* _fifo_handler;

static const int      giMaxEventCodes   = 64; // max number of event code configurations
static const int      giMaxPulses       = 10; 
static const int      giMaxOutputMaps   = 80; 
static const int      giMaxCalibCycles  = 500;
static const unsigned TERMINATOR        = 1;
static EvgrBoardInfo < Evr > *erInfoGlobal; // yuck

class EvrAction:public Action
{
public:
  EvrAction(Evr & er):_er(er)
  {
  }
  Evr & _er;
};

class EvrL1Action:public EvrAction
{
public:
  EvrL1Action(Evr & er, const Src& src, Appliance* app) :
    EvrAction(er),
    _occPool        (sizeof(Occurrence),1), 
    _app            (app) {}

  Transition* fire(Transition* tr) 
  { 
    return _fifo_handler ? _fifo_handler->disable(tr) : tr;
  }

  InDatagram *fire(InDatagram * in)
  {
    InDatagram* out = in;
    if (_fifo_handler) {
      out = _fifo_handler->l1accept(in);
      if (out->datagram().xtc.damage.value() & (1<<Damage::OutOfOrder)) {
        Pds::Occurrence* occ = new (&_occPool)
          Pds::Occurrence(Pds::OccurrenceId::ClearReadout);
        _app->post(occ);
      }
    }
    return out;
  }
    
private:
  GenericPool _occPool;
  Appliance*  _app;
};

class EvrEnableAction:public EvrAction
{
public:
  EvrEnableAction(Evr&       er,
      Appliance& app) : EvrAction(er), _app(app) {}
    
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
    maxNumEventCodes * sizeof(EvrConfigType::EventCodeType) +
    maxNumPulses     * sizeof(EvrConfigType::PulseType) + 
    maxNumOutputMaps * sizeof(EvrConfigType::OutputMapType));
}

class EvrConfigManager
{
public:
  EvrConfigManager(Evr & er, CfgClientNfs & cfg, Appliance& app, bool bTurnOffBeamCodes):
    _er (er),
    _cfg(cfg),
    _cfgtc(_evrConfigType, cfg.src()),
    _configBuffer(
      new char[ giMaxCalibCycles* evrConfigSize( giMaxEventCodes, giMaxPulses, giMaxOutputMaps ) ] ),
    _cur_config(0),
    _end_config(0),
    _occPool   (new GenericPool(sizeof(UserMessage),1)),
    _app       (app),
    _bTurnOffBeamCodes(_bTurnOffBeamCodes)
  {
  }

   ~EvrConfigManager()
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
    if (!_cur_config)
      return;

    bool slacEvr     = reinterpret_cast<uint32_t*>(&_er)[11] == 0x1f;
    unsigned nerror  = 0;
    UserMessage* msg = new(_occPool) UserMessage;
    msg->append(DetInfo::name(static_cast<const DetInfo&>(_cfgtc.src)));
    msg->append(":");

    const EvrConfigType& cfg = *_cur_config;

    printf("Configuring evr : %d/%d/%d\n",
           cfg.npulses(),cfg.neventcodes(),cfg.noutputs());

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
      omask |= 1<<cfg.output_map(k).conn_id();
    omask = ~omask;

    for (unsigned k = 0; k < EVR_MAX_UNIVOUT_MAP; k++) {
      if ((omask >> k)&1 && !(k>9 && !slacEvr))
        _er.SetUnivOutMap(k, EVR_MAX_PULSES-1);
    }

    // setup map ram
    int ram = 0;
    int enable = 1;

    for (unsigned k = 0; k < cfg.npulses(); k++)
    {
      const EvrConfigType::PulseType & pc = cfg.pulse(k);
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
      const EvrConfigType::OutputMapType & map = cfg.output_map(k);
      unsigned conn_id = map.conn_id();

      if (conn_id>9 && !slacEvr) {
        nerror++;
        char buff[64];
        sprintf(buff,"EVR output %d out of range [0-9].\n", conn_id);
        msg->append(buff);
      }

      switch (map.conn())
      {
      case EvrConfigType::OutputMapType::FrontPanel:
        _er.SetFPOutMap(conn_id, map.map());
        break;
      case EvrConfigType::OutputMapType::UnivIO:
        _er.SetUnivOutMap(conn_id, map.map());
        break;
      }

      printf("output %d : %d %x\n", k, conn_id, map.map());
    }
    
    /*
     * enable event codes, and setup each event code's pulse mapping
     */
    for (unsigned int uEventIndex = 0; uEventIndex < cfg.neventcodes(); uEventIndex++ )
    {
      const EvrConfigType::EventCodeType& eventCode = cfg.eventcode(uEventIndex);
              
      _er.SetFIFOEvent(ram, eventCode.code(), enable);
      
      unsigned int  uPulseBit     = 0x0001;
      uint32_t      u32TotalMask  = ( eventCode.maskTrigger() | eventCode.maskSet() | eventCode.maskClear() );
      
      for ( int iPulseIndex = 0; iPulseIndex < EVR_MAX_PULSES; iPulseIndex++, uPulseBit <<= 1 )
      {
        if ( (u32TotalMask & uPulseBit) == 0 ) continue;
        
        _er.SetPulseMap(ram, eventCode.code(), 
          ((eventCode.maskTrigger() & uPulseBit) != 0 ? iPulseIndex : -1 ),
          ((eventCode.maskSet()     & uPulseBit) != 0 ? iPulseIndex : -1 ),
          ((eventCode.maskClear()   & uPulseBit) != 0 ? iPulseIndex : -1 )
          );          
      }

      printf("event %d : %d %x/%x/%x readout %d group %d\n",
       uEventIndex, eventCode.code(), 
       eventCode.maskTrigger(),
       eventCode.maskSet(),
       eventCode.maskClear(),
       (int) eventCode.isReadout(),
       eventCode.readoutGroup());       
    }
    
    if (!_bTurnOffBeamCodes)
    {
      _er.SetFIFOEvent(ram, EvrManager::EVENT_CODE_BEAM,  enable);
      _er.SetFIFOEvent(ram, EvrManager::EVENT_CODE_BYKIK, enable);
      _er.SetFIFOEvent(ram, EvrSyncMaster::EVENT_CODE_SYNC, enable);
    }
    
    _er.SetFIFOEvent(ram, TERMINATOR, enable);

    unsigned dummyram = 1;
    _er.MapRamEnable(dummyram, 1);

    if (_fifo_handler)
      _fifo_handler->set_config( &cfg );

    if (nerror) {
      _app.post(msg);
      _cfgtc.damage.increase(Damage::UserDefined);
    }
    else
      delete msg;
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

    int len = _cfg.fetch(*tr, _evrConfigType, _configBuffer);
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

    _cur_config = reinterpret_cast<const EvrConfigType*>(_configBuffer);
    _end_config = _configBuffer+len;
    _cfgtc.extent = sizeof(Xtc) + _cur_config->size();

    delete msg;
    _cfgtc.damage = 0;
  }

  void advance()
  {
    int len = _cur_config->size();
    const char* nxt_config = reinterpret_cast<const char*>(_cur_config)+len;
    if (nxt_config < _end_config)
      _cur_config = reinterpret_cast<const EvrConfigType*>(nxt_config);
    else
      _cur_config = reinterpret_cast<const EvrConfigType*>(_configBuffer);
    _cfgtc.extent = sizeof(Xtc) + _cur_config->size();
  }

  void enable()
  {
    _er.IrqEnable(EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_EVENT);
    _er.EnableFIFO(1);
    _er.Enable(1);
  }

  void disable()
  {
    _er.IrqEnable(0);
    _er.Enable(0);
    _er.EnableFIFO(0);
  }
  
private:
  Evr&            _er;
  EvrCfgClient    _cfg;
  Xtc             _cfgtc;
  char *          _configBuffer;
  const EvrConfigType* _cur_config;
  const char*          _end_config;
  GenericPool*         _occPool;
  Appliance&           _app;
  bool                 _bTurnOffBeamCodes;
};

class EvrConfigAction: public Action {
public:
  EvrConfigAction(EvrConfigManager& cfg) : _cfg(cfg) {}
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
  EvrConfigManager& _cfg;
};

class EvrBeginCalibAction: public Action {
public:
  EvrBeginCalibAction(EvrConfigManager& cfg) : _cfg(cfg) {}
public:
  Transition* fire(Transition* tr) { 
    _cfg.configure(); 
    _cfg.enable(); 
      
    // sleep for 4 millisecond to update fiducial from MapRam 1
    timeval timeSleepMicro = {0, 4000}; // 2 milliseconds  
    select( 0, NULL, NULL, NULL, &timeSleepMicro);       
      
    return tr; }
  InDatagram* fire(InDatagram* dg) { _cfg.insert(dg); return dg; }
private:
  EvrConfigManager& _cfg;
};

class EvrEndCalibAction: public Action
{
public:
  EvrEndCalibAction(EvrConfigManager& cfg): _cfg(cfg) {}
public:
  Transition *fire(Transition * tr) { 
    _cfg.disable(); 
    if (_fifo_handler)
      _fifo_handler->endcalib(tr);
    return tr; 
  }
  InDatagram *fire(InDatagram * dg) { _cfg.advance(); return dg; }
private:
  EvrConfigManager& _cfg;
};


class EvrAllocAction:public Action
{
public:
  EvrAllocAction(CfgClientNfs& cfg, 
                 Evr&          er,
                 Appliance&         app,
                 EvrConfigManager&  cmgr) :
    _cfg(cfg), _er(er), _app(app), _cmgr(cmgr),
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
    int  iMaxGroup  = 0;
    for (unsigned n = 0; n < nnodes; n++)
    {
      const Node *node = alloc.allocation().node(n);
      if (node->level() == Level::Segment) 
      {
        if (node->group() > iMaxGroup)
          iMaxGroup = node->group();
      }
    }
    
    //
    //  Test if we own the primary EVR for this partition
    //
    int pid = getpid();
    bool lmaster = true;
    for (unsigned n = 0; n < nnodes; n++)
    {
      const Node *node = alloc.allocation().node(n);
      if (node->level() == Level::Segment) 
      {
        if (node->pid() == pid)
        {
    if (lmaster) 
      {
              printf("Found master EVR\n");
              _fifo_handler = new EvrMasterFIFOHandler(
                         _er,
                   _cfg.src(),
                   _app,
                   alloc.allocation().partitionid(),
                         iMaxGroup,
                   _task);
      }
    else
      {
              printf("Found slave EVR\n");
              _fifo_handler = new EvrSlaveFIFOHandler(
                        _er, 
                  _app,
                  alloc.allocation().partitionid(),
                  _task,
                                                      _sync_task);
      }
    break;
        }
        else
    lmaster = false;
      } // if (node->level() == Level::Segment) 
    } // for (unsigned n = 0; n < nnodes; n++)

    return tr;
  }
private:
  CfgClientNfs& _cfg;
  Evr&          _er;
  Appliance&    _app;
  EvrConfigManager& _cmgr;
  Task*         _task;
  Task*         _sync_task;
};

class EvrShutdownAction:public Action
{
public:
  EvrShutdownAction() {}
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
  void evrmgr_sig_handler(int parm)
  {
    Evr & er = erInfoGlobal->board();
    FIFOEvent fe;
    while( ! er.GetFIFOEvent(&fe) )
      if (_fifo_handler)
        _fifo_handler->fifo_event(fe);
    int fdEr = erInfoGlobal->filedes();
    er.IrqHandled(fdEr);
  }
}

Appliance & EvrManager::appliance()
{
  return _fsm;
}

EvrManager::EvrManager(EvgrBoardInfo < Evr > &erInfo, CfgClientNfs & cfg, bool bTurnOffBeamCodes):
  _er(erInfo.board()), _fsm(*new Fsm), _bTurnOffBeamCodes(bTurnOffBeamCodes)
{
  EvrConfigManager* cmgr = new EvrConfigManager(_er, cfg, _fsm, bTurnOffBeamCodes);

  _fsm.callback(TransitionId::Map            , new EvrAllocAction     (cfg,_er,_fsm, *cmgr));
  _fsm.callback(TransitionId::Unmap          , new EvrShutdownAction);
  _fsm.callback(TransitionId::Configure      , new EvrConfigAction    (*cmgr));
  _fsm.callback(TransitionId::BeginCalibCycle, new EvrBeginCalibAction(*cmgr));
  _fsm.callback(TransitionId::EndCalibCycle  , new EvrEndCalibAction  (*cmgr));
  _fsm.callback(TransitionId::Enable         , new EvrEnableAction    (_er,_fsm));
  _fsm.callback(TransitionId::L1Accept       , new EvrL1Action(_er, cfg.src(), &_fsm));

  _er.IrqAssignHandler(erInfo.filedes(), &evrmgr_sig_handler);
  erInfoGlobal = &erInfo;

  // Unix signal support
  struct sigaction int_action;

  int_action.sa_handler = EvrManager::sigintHandler;
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

EvrManager::~EvrManager()
{
}

void EvrManager::sigintHandler(int iSignal)
{
  printf("signal %d received\n", iSignal);

  if (erInfoGlobal && erInfoGlobal->mapped())
  {
    Evr & er = erInfoGlobal->board();
    printf("Stopping triggers and multicast... ");
    // switch to the "dummy" map ram so we can still get eventcodes 0x70,0x71,0x7d for timestamps
    unsigned dummyram=1;
    er.MapRamEnable(dummyram,1);
    printf("stopped\n");
  }
  else
  {
    printf("evr not mapped\n");
  }
  exit(0);
}

const int EvrManager::EVENT_CODE_BEAM;  // value is defined in the header file
const int EvrManager::EVENT_CODE_BYKIK; // value is defined in the header file
