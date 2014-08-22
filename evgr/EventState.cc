#include "pds/evgr/EventState.hh"
#include "pds/evgr/EvrL1Data.hh"
#include "pds/evgr/EvrDefs.hh"

using namespace Pds;

EventState::EventState() :
  uMaskReadout       (0),
  ncommands          (0)
{
 clear(); 
}

EventState::~EventState() 
{
}

void EventState::clear()
{
  memset(_codes,0,sizeof(_codes));
  clearFifoEvents();
}

void EventState::clearFifoEvents()
{
  _L1DataUpdated.clearFifoEvents();
  _L1DataLatch  .clearFifoEvents();
  _L1DataLatchQ .clearFifoEvents();
}

void EventState::configure(const ndarray<const EventCodeType,1>& c)
{
  clear();
  
  for(unsigned i=0; i<c.shape()[0]; i++) {
    const EventCodeType& eventcode = c[i];
    if (eventcode.code() >= guNumTypeEventCode) {
      printf( "EventState::configure(): event code out of range: %d\n", eventcode.code() );
      continue;
    }
    _codes[eventcode.code()].configure(eventcode);
    int iRelease = -int(eventcode.reportWidth());
    if (iRelease > 0)
      _codes[iRelease].bRelease=true;
  }

  _codes[EVENT_CODE_BEAM ].bSpecial=true;
  _codes[EVENT_CODE_BYKIK].bSpecial=true;
  _codes[EVENT_CODE_ALKIK].bSpecial=true;
}

void EventState::configure(const ndarray<const EvrData::SrcEventCode,1>& c)
{
  clear();
  
  for(unsigned i=0; i<c.shape()[0]; i++) {
    const EvrData::SrcEventCode& eventcode = c[i];
    if (eventcode.code() >= guNumTypeEventCode) {
      printf( "EventState::configure(): event code out of range: %d\n", eventcode.code() );
      continue;
    }
    _codes[eventcode.code()].configure(eventcode);
  }
}

void EventState::update(const FIFOEvent& fe)
{
  EventCodeState& codeState = _codes[fe.EventCode];

  if (codeState.iDefReportWidth == 0) {
    if (codeState.bSpecial)
      _L1DataUpdated.addSpecialEvent( fe );
    return;
  }

  if (codeState.uMaskReadout) {  //  Readout 
    uMaskReadout |= codeState.uMaskReadout;
    _L1DataUpdated.addFifoEvent( fe );
    return;
  }

  if (codeState.bCommand) {      //   Command
    if (ncommands < giMaxCommands)
      commands[ncommands++] = fe.EventCode;
    else
      printf("Dropped command %d\n",fe.EventCode);
  }

  if (codeState.bRelease) {     //   Latched
    _L1DataLatchQ.checkLatch(fe,_codes);
    _L1DataLatch .checkLatch(fe,_codes);
  }

  if (codeState.iReportWidth > 0) {   // Control-Transient
    if (codeState.iReportDelayQ <= 0) {     // Delay expired
      _L1DataLatchQ.updateFifoEvent( fe );  
      codeState.iReportDelayQ = codeState.iDefReportDelay;
    }
    else {   // The eventcode has appeared thrice and the delay has not expired from the first
      printf("EvrMasterFIFOHandler::fifo_event(): Can't queue eventcode %d\n",
	     fe.EventCode);
    }
  }
  else {                              // Control-Latch
    _L1DataLatch.updateFifoEvent( fe );
    codeState.iReportDelay = codeState.iDefReportDelay;
    codeState.iReportWidth = codeState.iDefReportWidth;
  }
}

//
//  Advance one readout event
//
void EventState::advance(EvrL1Data&       l1Data,
			 const ClockTime& ctime,
			 bool             bIncomplete)
{
  EvrDataUtil L1DataFinal;

  _L1DataLatchQ .advanceQ( _L1DataLatch, _codes );
  _L1DataLatch  .advance ( L1DataFinal , _codes );
  L1DataFinal   .insert  ( _L1DataUpdated );

  L1DataFinal.write(l1Data.nextWrite(ctime,
				     L1DataFinal.full(),
				     bIncomplete));

  _L1DataUpdated.clearFifoEvents();
}

