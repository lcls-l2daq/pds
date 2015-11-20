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
#include "EvrConfigManager.hh"
#include "EvrFifoServer.hh"

#include "pds/evgr/EvrMasterFIFOHandler.hh"
#include "pds/evgr/EvrSlaveFIFOHandler.hh"

#include "evgr/evr/evr.hh"

#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/client/Response.hh"
#include "pds/xtc/EvrDatagram.hh"
#include "pds/config/EvrConfigType.hh" // typedefs for the Evr config data types
#include "pds/config/CfgClientNfs.hh"

#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/utility/InletWire.hh"
#include "pds/management/SegmentLevel.hh"
//#include "pds/utility/InletWireServer.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/OccurrenceId.hh"
#include "pds/xtc/CDatagram.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TimeStamp.hh"

#include "pds/vmon/VmonEvr.hh"

using namespace Pds;

static bool _randomize_nodes = false;
static EvrFIFOHandler* _fifo_handler;

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
      if ((out = _fifo_handler->l1accept(in))) {
        if (!_outOfOrder && out->datagram().xtc.damage.value() & (1<<Damage::OutOfOrder)) {
          Pds::Occurrence* occ = new (&_occPool)
            Pds::Occurrence(Pds::OccurrenceId::ClearReadout);
          _app->post(occ);
          _outOfOrder = true;
        }
      }
    }
    return out;
  }

private:
  GenericPool _occPool;
  Appliance*  _app;
  bool        _outOfOrder;
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

class EvrConfigAction: public Action {
public:
  EvrConfigAction(EvrConfigManager& cfg) : _cfg(cfg) {}
public:
  Transition* fire(Transition* tr)
  {
    _cfg.fetch(tr);
    const EvrConfigType* cfg = _cfg.configure();

    if (_fifo_handler) {
      if (cfg) _fifo_handler->set_config( cfg );
      _fifo_handler->config(tr);
    }

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
    const EvrConfigType* cfg = _cfg.configure();

    if (_fifo_handler && cfg)
      _fifo_handler->set_config( cfg );

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
  EvrAllocAction(const Src&         src,
                 Evr&               er,
                 Appliance&         app,
                 EvrConfigManager&  cmgr,
                 EvrFifoServer*&    pSrv,
                 unsigned           module,
                 InletWire*&        pWire,
                 VmonEvr&           vmon) :
    _src(src), _er(er), _app(app), _cmgr(cmgr), _pSrv(pSrv),
    _module(module), _pWire(pWire),
    _vmon     (vmon),
    _task     (new Task(TaskObject("evrsync"))),
    _sync_task(new Task(TaskObject("slvsync")))
  {
    _fifo_handler = 0;
  }
  Transition *fire(Transition * tr)
  {
    const Allocate & alloc = reinterpret_cast < const Allocate & >(*tr);
    _cmgr.initialize(alloc.allocation());

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

        if (_pSrv == NULL)
        {
          _pSrv = new EvrFifoServer(_src);
          (_pWire)->add_input_nonblocking(_pSrv);
        }

        _fifo_handler = new EvrMasterFIFOHandler(
                   _er,
                   _src,
                   _app,
                   *_pSrv,
                   alloc.allocation().partitionid(),
                   iMaxGroup,
                   _module,
                   alloc.allocation().nnodes(Level::Event),
                   _randomize_nodes,
                   _task,
                   _vmon);


      }
      else
      {
        printf("Found slave EVR\n");
        _fifo_handler = new EvrSlaveFIFOHandler(
                  _er,
                  _app,
                  *_pSrv,
                  alloc.allocation().partitionid(),
                  iMaxGroup,
                  _module,
                  _task,
                  _sync_task,
                  _vmon);
        if (_pSrv)
        {
          (_pWire)->trim_input(_pSrv);
          _pSrv = NULL;
        }
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
  const Src&          _src;
  Evr&                _er;
  Appliance&          _app;
  EvrConfigManager&   _cmgr;
  EvrFifoServer*&     _pSrv;
  unsigned            _module;
  InletWire*&         _pWire;
  VmonEvr&            _vmon;
  Task*               _task;
  Task*               _sync_task;
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

class EvrOccResponse : public Response {
public:
  EvrOccResponse(EvrConfigManager& cfg) : _cfg(cfg) {}
public:
  Occurrence* fire(Occurrence* occ) {
    const EvrCommandRequest& req = *reinterpret_cast<const EvrCommandRequest*>(occ);
    _cfg.handleCommandRequest(req.eventCodes());
    return 0;
  }
private:
  EvrConfigManager& _cfg;
};


extern "C"
{
  void evrmgr_sig_handler(int parm)
  {
    Evr & er = erInfoGlobal->board();
    if (er.GetIrqFlags() & EVR_IRQFLAG_FIFOFULL) {
      _fifo_handler->fifo_full();
      er.ClearIrqFlags(EVR_IRQFLAG_FIFOFULL);
    }
    Pds::FIFOEvent fe;
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

Server& EvrManager::server()
{
  return *_server;
}

static struct sigaction old_actions[64];

EvrManager::EvrManager(EvgrBoardInfo < Evr > &erInfo, CfgClientNfs & cfg, bool bTurnOffBeamCodes, unsigned module):
  _er(erInfo.board()), _fsm(*new Fsm),
  _server(new EvrFifoServer(cfg.src())),
  _bTurnOffBeamCodes(bTurnOffBeamCodes),
  _module(module),
  _pWire(NULL),
  _vmon (new VmonEvr(cfg.src()))
{
  EvrConfigManager* cmgr = new EvrConfigManager(_er, cfg, _fsm, bTurnOffBeamCodes);

  _fsm.callback(TransitionId::Map            , new EvrAllocAction     (cfg.src(),_er,_fsm, *cmgr, _server, _module, _pWire, *_vmon));
  _fsm.callback(TransitionId::Unmap          , new EvrShutdownAction);
  _fsm.callback(TransitionId::Configure      , new EvrConfigAction    (*cmgr));
  _fsm.callback(TransitionId::BeginCalibCycle, new EvrBeginCalibAction(*cmgr));
  _fsm.callback(TransitionId::EndCalibCycle  , new EvrEndCalibAction  (*cmgr));
  _fsm.callback(TransitionId::Enable         , new EvrEnableAction    (_er,_fsm));
  _fsm.callback(TransitionId::L1Accept       , new EvrL1Action(_er, cfg.src(), &_fsm));

  _fsm.callback(OccurrenceId::EvrCommandRequest, new EvrOccResponse   (*cmgr));

  _er.IrqAssignHandler(erInfo.filedes(), &evrmgr_sig_handler);
  erInfoGlobal = &erInfo;

  // Unix signal support
  struct sigaction int_action;

  int_action.sa_handler = EvrManager::sigintHandler;
  sigemptyset(&int_action.sa_mask);
  int_action.sa_flags = 0;
  int_action.sa_flags |= SA_RESTART;

#define REGISTER(t) {                                               \
    if (sigaction(t, &int_action, &old_actions[t]) > 0)             \
      printf("Couldn't set up #t handler\n");                       \
  }

  REGISTER(SIGINT);
  REGISTER(SIGKILL);
  REGISTER(SIGSEGV);
  REGISTER(SIGABRT);
  REGISTER(SIGTERM);

#undef REGISTER
}

EvrManager::~EvrManager()
{
}

void EvrManager::sigintHandler(int sig_no)
{
  printf("signal %d received\n", sig_no);

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

  sigaction(sig_no,&old_actions[sig_no],NULL);
  //  raise(sig_no);
  kill(getpid(),sig_no);
}

void EvrManager::randomize_nodes(bool v) { _randomize_nodes=v; }
