#include <stdio.h>

#include "EvrDataUtil.hh"

namespace Pds
{
  
EvrDataUtil::EvrDataUtil(uint32_t u32NumFifoEvents, const FIFOEvent* lFifoEvent) :
  EvrDataType(u32NumFifoEvents,lFifoEvent)
{
}

EvrDataUtil::EvrDataUtil(const EvrDataType& dataCopy):
  EvrDataType(dataCopy)
{
}
  
void EvrDataUtil::printFifoEvents() const
{
  printf( "# of Fifo Events: %u\n", _u32NumFifoEvents );
  
  for ( unsigned int iEventIndex=0; iEventIndex< _u32NumFifoEvents; iEventIndex++ )
  {
    const FIFOEvent& event = fifoEvent(iEventIndex);
    printf( "[%02u] Event Code %u  TimeStampHigh 0x%x  TimeStampLow 0x%x\n",
      iEventIndex, event.EventCode, event.TimestampHigh, event.TimestampLow );
  }
}

// return the number of total fifo events, including the new one
unsigned int EvrDataUtil::addFifoEvent( const FIFOEvent& fifoEvent )
{
  FIFOEvent* pFifoEventNew = (FIFOEvent*) ((char*) this + EvrDataType::size()); 
  *pFifoEventNew            = fifoEvent;  
  _u32NumFifoEvents++;   
  return _u32NumFifoEvents;
}

// return the index to the updated fifo event
unsigned int EvrDataUtil::updateFifoEvent( const FIFOEvent& fifoEvent )
{
  FIFOEvent*    pFifoEventUpdate = (FIFOEvent*) (this+1);  
  FIFOEvent*    pFifoEventCur    = pFifoEventUpdate;
  unsigned int  iNumTotalEvents  = 0;
  
  for ( unsigned int iEvent = 0 ; iEvent < _u32NumFifoEvents; iEvent++, pFifoEventCur++ )
  {
    if ( pFifoEventCur->EventCode == fifoEvent.EventCode )
      continue;

    // Condition: pFifoEvent->EventCode != fifoEvent.EventCode
      
    if ( pFifoEventUpdate != pFifoEventCur )
      *pFifoEventUpdate = *pFifoEventCur;
      
    ++pFifoEventUpdate;  
    ++iNumTotalEvents;
  }
  
  *pFifoEventUpdate = fifoEvent;    
  ++iNumTotalEvents;
  
  _u32NumFifoEvents = iNumTotalEvents;
  return (_u32NumFifoEvents-1);
}

static const uint32_t giMarkEventDel = 0xFFFFFFFF;

void EvrDataUtil::markEventAsDeleted( unsigned int iEventIndex )
{
  FIFOEvent* pFifoEvent = (FIFOEvent*) (this+1) + iEventIndex;    
  pFifoEvent->EventCode = giMarkEventDel; // Mark as deleted 
}

// return the new total number of events after the purge
unsigned int EvrDataUtil::PurgeDeletedEvents()
{  
  FIFOEvent* pFifoEventUpdate = (FIFOEvent*) (this+1);  
  FIFOEvent* pFifoEventCur    = pFifoEventUpdate;

  unsigned int iNumTotalEvents = 0;
  
  for ( unsigned int iEvent = 0 ; iEvent < _u32NumFifoEvents; iEvent++, pFifoEventCur++ )
  {
    if ( pFifoEventCur->EventCode != giMarkEventDel )
    {
      if ( pFifoEventUpdate != pFifoEventCur )
        *pFifoEventUpdate = *pFifoEventCur;
      
      ++pFifoEventUpdate;  
      ++iNumTotalEvents;
    }
  }
  
  _u32NumFifoEvents = iNumTotalEvents;
  return _u32NumFifoEvents;
}

// return the number of total fifo events, after update
unsigned int EvrDataUtil::removeTailEvent() 
{
  if ( _u32NumFifoEvents > 0 )
    --_u32NumFifoEvents;
    
  return _u32NumFifoEvents;
}

void EvrDataUtil::clearFifoEvents()
{
  _u32NumFifoEvents = 0;  
}

unsigned int EvrDataUtil::size(int iMaxNumFifoEvents)
{
  return ( sizeof(EvrDataType) + iMaxNumFifoEvents * sizeof(FIFOEvent) );
}

} // namespace Pds
