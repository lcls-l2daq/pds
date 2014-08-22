#include <stdio.h>

#include "pds/evgr/EvrDataUtil.hh"
#include "pds/evgr/EventCodeState.hh"

using namespace Pds;
 
enum { giMaxNumFifoEvent  = 32 };
 
EvrDataUtil::EvrDataUtil() : _full(false) {}

unsigned EvrDataUtil::numFifoEvents() const { return _data.size(); }

void EvrDataUtil::clearFifoEvents()
{
  _data.clear();
  _full = false;
}

void EvrDataUtil::printFifoEvents() const
{
  printf( "# of Fifo Events: %zu\n", _data.size() );
  
  unsigned i=0;
  for(std::list<FifoEventT>::const_iterator it=_data.begin();
      it!=_data.end(); it++) {
    const FIFOEvent& event = it->event;
    printf( "[%02u] Event Code %u  TimeStampHigh 0x%x  TimeStampLow 0x%x\n",
            i++, event.EventCode, event.TimestampHigh, event.TimestampLow );
  }
}

void EvrDataUtil::addFifoEvent( const FIFOEvent& fifoEvent )
{
  if (numFifoEvents() < giMaxNumFifoEvent)
    _data.push_back(FifoEventT(fifoEvent,false));
  else
    _full=true;
}

void EvrDataUtil::addSpecialEvent( const FIFOEvent& fifoEvent )
{
  while(!_data.empty()) {
    const FifoEventT& t = _data.back();
    if (t.event.TimestampHigh==fifoEvent.TimestampHigh)
      break;
    if (!t.transient)
      break;
    _data.pop_back();
  }

  if (numFifoEvents() < giMaxNumFifoEvent)
    _data.push_back(FifoEventT(fifoEvent,true));
  else
    _full = true;
}

void EvrDataUtil::updateFifoEvent( const FIFOEvent& fifoEvent )
{
  for(std::list<FifoEventT>::iterator it=_data.begin();
      it!=_data.end(); it++)
    if (it->event.EventCode == fifoEvent.EventCode) {
      it->event = fifoEvent;
      return;
    }

  if (numFifoEvents() < giMaxNumFifoEvent)
    _data.push_back(FifoEventT(fifoEvent,false));
  else
    _full = true;
}

void EvrDataUtil::checkLatch ( const FIFOEvent&      fifoEvent,
			       const EventCodeState* codes)
{
  const int releaseCode = -int(fifoEvent.EventCode);

  for(std::list<FifoEventT>::iterator it=_data.begin();
      it!=_data.end(); ) {
    const EventCodeState& code = codes[it->event.EventCode];
    if (code.iReportWidth == releaseCode)
      it = _data.erase(it);
    else
      it++;
  }
}

void EvrDataUtil::advanceQ( EvrDataUtil&    next,
			    EventCodeState* codes )
{
  for(std::list<FifoEventT>::iterator it=_data.begin();
      it!=_data.end();) {
    EventCodeState& codeState = codes[it->event.EventCode];
    if ( codeState.iReportWidth  <= 0 ||
	 codeState.iReportDelayQ <= 0 ) {
      next  .updateFifoEvent( it->event );
      it = _data.erase(it);
      codeState.iReportDelay  = codeState.iReportDelayQ;
      codeState.iReportWidth  = codeState.iDefReportWidth;
      codeState.iReportDelayQ = 0;
      continue;
    }

    --codeState.iReportDelayQ;
    it++;
  }
  next._full = next._full || _full;
}

void EvrDataUtil::advance ( EvrDataUtil&    next,
			    EventCodeState* codes )
{
  for(std::list<FifoEventT>::iterator it=_data.begin();
      it!=_data.end();) {
    EventCodeState& codeState = codes[it->event.EventCode];
    if ( codeState.iReportDelay > 0 ) {
      --codeState.iReportDelay;
    }
    else {
      next.addFifoEvent( it->event );
      
      /*
       * Test if any control-transient event codes have expired.
       */
      if (codeState.iReportWidth>=0) { // control-latch codes will bypass this test, since its report width = -1 * (release code)
	if ( --codeState.iReportWidth <= 0 ) {
	  it = _data.erase(it);
	  continue;
	}
      }
    }
    it++;
  }
  next._full = next._full || _full;
}

void EvrDataUtil::insert( const EvrDataUtil& input )
{
  _data.insert(_data.end(), input._data.begin(), input._data.end());
  if (numFifoEvents() >= giMaxNumFifoEvent) {
    _full |= true;
    _data.resize(giMaxNumFifoEvent);
  }
  _full = _full || input._full;
}

EvrDataType* EvrDataUtil::write(void* p)
{
  EvrDataType* r = new(p) EvrDataType(numFifoEvents(),NULL);
  EvrData::FIFOEvent* q = reinterpret_cast<EvrData::FIFOEvent*>(r+1);
  for(std::list<FifoEventT>::const_iterator it=_data.begin();
      it!=_data.end(); it++)
    *q++ = reinterpret_cast<const EvrData::FIFOEvent&>(it->event);
  return r;
}
