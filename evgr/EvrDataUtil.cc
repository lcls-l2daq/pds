#include <stdio.h>

#include "EvrDataUtil.hh"

namespace Pds
{
  
using namespace EvrData;
  
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

void EvrDataUtil::clearFifoEvents()
{
  _u32NumFifoEvents = 0;  
}

unsigned int EvrDataUtil::size(int iMaxNumFifoEvents)
{
  return ( sizeof(EvrDataType) + iMaxNumFifoEvents * sizeof(FIFOEvent) );
}

} // namespace Pds
