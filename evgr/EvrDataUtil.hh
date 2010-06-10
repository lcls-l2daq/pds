#include "pds/config/EvrConfigType.hh" // typedefs for the Evr config data types

namespace Pds
{
  
class EvrDataUtil : public EvrDataType
{  
public:
  /* public functions*/  
  EvrDataUtil(uint32_t u32NumFifoEvents, const FIFOEvent* lFifoEvent);
  EvrDataUtil(const EvrDataType& dataCopy);

  unsigned int  addFifoEvent      ( const FIFOEvent& fifoEvent ); // return the number of total fifo events, including the new one  
  unsigned int  updateFifoEvent   ( const FIFOEvent& fifoEvent ); // return the index to the updated fifo event
  void          markEventAsDeleted( unsigned int iEventIndex );
  unsigned int  PurgeDeletedEvents(); // return the new total number of events after the purge
  

  unsigned int  removeTailEvent (); // return the number of total fifo events, after update
  void          clearFifoEvents ();
  void          printFifoEvents () const;

  using EvrDataType::size; // unhide the size() member function
  
  /*
   * public static function
   */  
  static unsigned int size(int iMaxNumFifoEvents);   
}; // class EvrDataUtil

} // namespace Pds
