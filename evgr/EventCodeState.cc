#include "pds/evgr/EventCodeState.hh"
#include <stdio.h>

using namespace Pds;

EventCodeState::EventCodeState()
{
  uMaskReadout=0;
  bCommand    =false;
  bRelease    =false;
  iDefReportDelay=0;
  iDefReportWidth=0;
  iReportWidth =0;
  iReportDelayQ=0;
  iReportDelay =0;
}

EventCodeState::~EventCodeState() {}

void EventCodeState::configure(const EventCodeType& code)
{
  uMaskReadout   |= code.isReadout() ? (1<<code.readoutGroup()) : 0;
  bCommand        = code.isCommand();
  iDefReportDelay = code.reportDelay();
  iDefReportWidth = code.isLatch() ? -code.releaseCode() : code.reportWidth();

  printf("EventCode %d  readout %c group %d mask 0x%x command %c  latch %c  delay %d  width %d\n",
   code.code(),
   code.isReadout() ? 'Y':'n',
   code.readoutGroup(),
   uMaskReadout,
   code.isCommand() ? 'Y':'n',
   code.isLatch  () ? 'Y':'n',
   code.reportDelay(),
   code.reportWidth());
}

void EventCodeState::configure(const EvrData::SrcEventCode& code)
{
  uMaskReadout   |= 1<<code.readoutGroup();
  bCommand        = false;
  iDefReportDelay = 0;
  iDefReportWidth = 1;
}
