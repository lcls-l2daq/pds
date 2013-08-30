#include <stdio.h>

#include "EvrDataUtil.hh"

using Pds::EvrData::FIFOEvent;

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
  printf( "# of Fifo Events: %u\n", numFifoEvents() );
  
  ndarray<const EvrData::FIFOEvent,1> events = fifoEvents();
  for ( unsigned int iEventIndex=0; iEventIndex< events.shape()[0]; iEventIndex++ )
  {
    const FIFOEvent& event = events[iEventIndex];
    printf( "[%02u] Event Code %u  TimeStampHigh 0x%x  TimeStampLow 0x%x\n",
            iEventIndex, event.eventCode(), event.timestampHigh(), event.timestampLow() );
  }
}

// return the number of total fifo events, including the new one
unsigned int EvrDataUtil::addFifoEvent( const FIFOEvent& fifoEvent )
{
  FIFOEvent* pFifoEventNew = (FIFOEvent*) ((char*) this + size()); 
  *pFifoEventNew            = fifoEvent;  
  return (new (this) EvrDataType(numFifoEvents()+1))->numFifoEvents();
}

// return the index to the updated fifo event
unsigned int EvrDataUtil::updateFifoEvent( const FIFOEvent& fifoEvent )
{
  FIFOEvent*    pFifoEvent = (FIFOEvent*) (this+1);  
  
  for ( unsigned int iEvent = 0 ; iEvent < numFifoEvents(); iEvent++, pFifoEvent++ )
  {
    if ( pFifoEvent->eventCode() == fifoEvent.eventCode() )
    {
      *pFifoEvent = fifoEvent;
      return iEvent;
    }
  }

  *pFifoEvent = fifoEvent;
  return (new (this) EvrDataType(numFifoEvents()+1))->numFifoEvents();
}

// return the index to the updated fifo event
int EvrDataUtil::updateFifoEventCheck( const FIFOEvent& fifoEvent, unsigned int iMaxSize )
{
  FIFOEvent*    pFifoEvent = (FIFOEvent*) (this+1);  
  
  for ( unsigned int iEvent = 0 ; iEvent < numFifoEvents(); iEvent++, pFifoEvent++ )
  {
    if ( pFifoEvent->eventCode() == fifoEvent.eventCode() )
    {
      *pFifoEvent = fifoEvent;
      return iEvent;
    }
  }

  if ( numFifoEvents() >= iMaxSize )
    return -1;
    
  *pFifoEvent = fifoEvent;
  return (new (this) EvrDataType(numFifoEvents()+1))->numFifoEvents();
}

static const uint32_t giMarkEventDel = 0xFFFFFFFF;

void EvrDataUtil::markEventAsDeleted( unsigned int iEventIndex )
{
  FIFOEvent* pFifoEvent = (FIFOEvent*) (this+1) + iEventIndex;    
  *new(pFifoEvent) FIFOEvent(pFifoEvent->timestampHigh(),
                             pFifoEvent->timestampLow(),
                             giMarkEventDel); // Mark as deleted 
}

// return the new total number of events after the purge
unsigned int EvrDataUtil::purgeDeletedEvents()
{  
  FIFOEvent* pFifoEventUpdate = (FIFOEvent*) (this+1);  
  FIFOEvent* pFifoEventCur    = pFifoEventUpdate;

  unsigned int iNumTotalEvents = 0;
  
  for ( unsigned int iEvent = 0 ; iEvent < numFifoEvents(); iEvent++, pFifoEventCur++ )
  {
    if ( pFifoEventCur->eventCode() != giMarkEventDel )
    {
      if ( pFifoEventUpdate != pFifoEventCur )
        *pFifoEventUpdate = *pFifoEventCur;
      
      ++pFifoEventUpdate;  
      ++iNumTotalEvents;
    }
  }
  
  return (new (this) EvrDataType(iNumTotalEvents))->numFifoEvents();
}

// return the number of total fifo events, after update
unsigned int EvrDataUtil::removeTailEvent() 
{
  if ( numFifoEvents() > 0 )
    *new(this) EvrDataType(numFifoEvents()-1);
    
  return numFifoEvents();
}

void EvrDataUtil::clearFifoEvents()
{
  *new(this) EvrDataType(0);
}

unsigned int EvrDataUtil::size(int iMaxNumFifoEvents)
{
  return ( sizeof(EvrDataType) + iMaxNumFifoEvents * sizeof(FIFOEvent) );
}

} // namespace Pds
