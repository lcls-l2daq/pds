#include "pds/evgr/EvrConfigManager.hh"
#include "pds/evgr/EvrManager.hh"  // EVENT_CODE_BEAM, ...
#include "pds/evgr/EvrSync.hh"     // EVENT_CODE_SYNC
#include "pds/utility/Appliance.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/service/GenericPool.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "evgr/evr/evr.hh"

#include <string>
#include <sstream>

using namespace Pds;

static const int      giMaxEventCodes   = 64; // max number of event code configurations
static const int      giMaxPulses       = 10; 
static const int      giMaxOutputMaps   = 80; 
static const int      giMaxCalibCycles  = 500;
static const unsigned TERMINATOR        = 1;

static unsigned int evrConfigSize(unsigned maxNumEventCodes, unsigned maxNumPulses, unsigned maxNumOutputMaps)
{
  return (sizeof(EvrConfigType) + 
    maxNumEventCodes * sizeof(EventCodeType) +
    maxNumPulses     * sizeof(PulseType) + 
    maxNumOutputMaps * sizeof(OutputMapType));
}

EvrConfigManager::EvrConfigManager(Evr&          er, 
				   CfgClientNfs& cfg, 
				   Appliance&    app, 
				   bool          bTurnOffBeamCodes):
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

EvrConfigManager::~EvrConfigManager()
{
  delete   _occPool;
  delete[] _configBuffer;
}

void EvrConfigManager::initialize(const Allocation& alloc) 
{
  _cfg.initialize(_alloc=alloc);
}

void EvrConfigManager::insert(InDatagram * dg)
{
  dg->insert(_cfgtc, _cur_config);
}

const EvrConfigType* EvrConfigManager::configure()
{
  if (!_cur_config)
    return 0;

  bool slacEvr     = (reinterpret_cast<uint32_t*>(&_er)[11]&0xff) == 0x1f;
  unsigned nerror  = 0;
  UserMessage* msg = new(_occPool) UserMessage;
  msg->append(DetInfo::name(static_cast<const DetInfo&>(_cfgtc.src)));
  msg->append(":");

  const EvrConfigType& cfg = *_cur_config;

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
  int ram = 0;
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

      //  Set negative delays to zero
      if (delay & (1<<31)) 
	{
	  printf("EVR pulse %d delay (%d) set to 0\n",
		 pc.pulseId(), (int)delay);
	  delay = 0;
	}

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
	  _er.SetFPOutMap(conn_id, Pds::EvrConfig::map(map));
	  break;
	case OutputMapType::UnivIO:
	  _er.SetUnivOutMap(conn_id, Pds::EvrConfig::map(map));
	  break;
	}

      printf("output %d : %d %x\n", k, conn_id, Pds::EvrConfig::map(map));
    }
    
  /*
   * enable event codes, and setup each event code's pulse mapping
   */
  for (unsigned int uEventIndex = 0; uEventIndex < cfg.neventcodes(); uEventIndex++ )
    {
      const EventCodeType& eventCode = cfg.eventcodes()[uEventIndex];
              
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
      _er.SetFIFOEvent(ram, EvrManager::EVENT_CODE_ALKIK, enable);
      _er.SetFIFOEvent(ram, EvrSyncMaster::EVENT_CODE_SYNC, enable);
    }
    
  _er.SetFIFOEvent(ram, TERMINATOR, enable);

  unsigned dummyram = 1;
  _er.MapRamEnable(dummyram, 1);

  if (nerror) {
    _app.post(msg);
    _cfgtc.damage.increase(Damage::UserDefined);
  }
  else
    delete msg;

  return &cfg;
}

//
//  Fetch the explicit EVR configuration and the Sequencer configuration.
//
void EvrConfigManager::fetch(Transition* tr)
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
  _cfgtc.extent = sizeof(Xtc) + Pds::EvrConfig::size(*_cur_config);

  //  Verify consistency in the group settings
  if (_validate_groups(msg)) {
    _app.post(msg);
    return;
  }

  delete msg;
  _cfgtc.damage = 0;
}

void EvrConfigManager::advance()
{
  int len = Pds::EvrConfig::size(*_cur_config);
  const char* nxt_config = reinterpret_cast<const char*>(_cur_config)+len;
  if (nxt_config < _end_config)
    _cur_config = reinterpret_cast<const EvrConfigType*>(nxt_config);
  else
    _cur_config = reinterpret_cast<const EvrConfigType*>(_configBuffer);
  _cfgtc.extent = sizeof(Xtc) + Pds::EvrConfig::size(*_cur_config);
}

void EvrConfigManager::enable()
{
  _er.IrqEnable(EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_EVENT);
  _er.EnableFIFO(1);
  _er.Enable(1);
}

void EvrConfigManager::disable()
{
  _er.IrqEnable(0);
  _er.Enable(0);
  _er.EnableFIFO(0);
}

void EvrConfigManager::handleCommandRequest(const std::vector<unsigned>& codes)
{
  printf("Received command request for codes: [");
  for(unsigned i=0; i<codes.size(); i++)
    printf(" %d",codes[i]);
  printf(" ]\n");

  char* configBuffer = 
    new char[ giMaxCalibCycles* evrConfigSize( giMaxEventCodes, giMaxPulses, giMaxOutputMaps ) ];

  char* p = configBuffer;
  for(const char* cur_config = _configBuffer;
      cur_config < _end_config; ) {
    const EvrConfigType& cfg = *reinterpret_cast<const EvrConfigType*>(cur_config);
    ndarray<EventCodeType,1> eventcodes = make_ndarray<EventCodeType>(cfg.neventcodes()+codes.size());
    memcpy(eventcodes.data(),cfg.eventcodes().data(),cfg.neventcodes()*sizeof(EventCodeType));

    unsigned ncodes = cfg.neventcodes();
    for(unsigned i=0; i<codes.size(); i++) {
      bool lFound=false;
      for(unsigned j=0; j<ncodes; j++) {
	EventCodeType& c = eventcodes[j];
	if (c.code()==codes[i]) {
	  lFound=true;
	  if (!(c.isReadout() || c.isCommand())) {
	    /*
              printf("Changing %03d %c %c %c %d %d %04x %04x %04x %s %d\n",
	      c.code(), c.isReadout(), c.isCommand(), c.isLatch(),
	      c.reportDelay(), c.reportWidth(), 
	      c.maskTrigger(), c.maskSet(), c.maskClear(),
	      c.desc(), c.readoutGroup());
	    */
	    new (&c) EventCodeType(c.code(), false, true, c.isLatch(), 
				   c.reportDelay(), c.reportWidth(), 
				   c.maskTrigger(), c.maskSet(), c.maskClear(),
				   c.desc(), c.readoutGroup());
	    /*
              printf("      to %03d %c %c %c %d %d %04x %04x %04x %s %d\n",
	      c.code(), c.isReadout(), c.isCommand(), c.isLatch(),
	      c.reportDelay(), c.reportWidth(), 
	      c.maskTrigger(), c.maskSet(), c.maskClear(),
	      c.desc(), c.readoutGroup());
	    */
	  }
	  break;
	}
      }
      if (!lFound) {
	EventCodeType& c = eventcodes[ncodes++];
	new (&c) EventCodeType(codes[i], false, true, false, 
			       0, 1,
			       0, 0, 0,
			       "CommandRequest",0);
	/*
          printf("Adding %03d %c %c %c %d %d %04x %04x %04x %s %d\n",
	  c.code(), c.isReadout(), c.isCommand(), c.isLatch(),
	  c.reportDelay(), c.reportWidth(), 
	  c.maskTrigger(), c.maskSet(), c.maskClear(),
	  c.desc(), c.readoutGroup());
	*/
      }
    }

    if (cur_config==reinterpret_cast<const char*>(_cur_config))
      _cur_config = reinterpret_cast<const EvrConfigType*>(p);

    EvrConfigType& newcfg = *new(p) EvrConfigType(ncodes, cfg.npulses(), cfg.noutputs(),
						  eventcodes.begin(), cfg.pulses().begin(), 
						  cfg.output_maps().begin(), cfg.seq_config());
    p += newcfg._sizeof();

    cur_config += cfg._sizeof();
  }

  delete[] _configBuffer;
  _configBuffer = configBuffer;
  _end_config   = p;
}

int EvrConfigManager::_validate_groups(UserMessage* msg)
{
  unsigned modid = static_cast<const Pds::DetInfo&>(_cfg.src()).devId();

  std::vector<unsigned> _omask(EventCodeType::MaxReadoutGroup+1);
  for(unsigned i=0; i<_alloc.nnodes(); i++) {
    const Node& n = *_alloc.node(i);
    printf("[%s]%x.%d  group %d  triggered %c  evr [%d]%d/%d\n",
	   Level::name(n.level()), n.ip(),n.pid(), 
	   n.group(),
	   n.triggered() ? 't':'f', modid, n.evr_module(), n.evr_channel());
    if (n.level()==Level::Segment && n.triggered() && n.evr_module()==modid)
      _omask[n.group()] |= 1<<n.evr_channel();
  }
  
  const char* p = reinterpret_cast<const char*>(_cur_config);
  while( p < _end_config ) {
    const EvrConfigType& c = *reinterpret_cast<const EvrConfigType*>(p);
    std::vector<unsigned> omask(EventCodeType::MaxReadoutGroup+1);
    for(unsigned k=0; k<c.neventcodes(); k++) {
      const EventCodeType& ec = c.eventcodes()[k];
      for(unsigned m=0; m<c.noutputs(); m++) {
	const OutputMapType& o = c.output_maps()[m];
	if (ec.maskTrigger()&(1<<o.source_id()))
	  omask[ec.readoutGroup()] |= 1<<o.conn_id();
      }
    }

    printf("_omask: ");
    for(unsigned g=0; g<_omask.size(); g++)
      printf("%08x ",_omask[g]);
    printf("\n omask: ");
    for(unsigned g=0; g<omask.size(); g++)
      printf("%08x ",omask[g]);
    printf("\n");

    std::ostringstream s;
    for(unsigned g=0; g<_omask.size(); g++)
      for(unsigned k=0; k<omask.size(); k++) {
	if (g==k) {
	  if ((_omask[g]&omask[k]) != _omask[g])
	    s << "Group " << g << " is inconsistent" << std::endl;
	}
	else if ((_omask[g]&omask[k]) != 0)
	  s << "Groups " << g << " and " << k << " overlap" << std::endl;
      }
    if (s.str().size()) {
      msg->append(s.str().c_str());
      return 1;
    }

    p += Pds::EvrConfig::size(c);
  }
  return 0;
}
