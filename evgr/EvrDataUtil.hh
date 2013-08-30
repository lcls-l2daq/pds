#ifndef Pds_EvrDataUtil_hh
#define Pds_EvrDataUtil_hh

#include "pds/config/EvrConfigType.hh" // typedefs for the Evr config data types

namespace Pds
{
  
class EvrDataUtil : public EvrDataType
{  
public:
  /* public functions*/  
  EvrDataUtil(uint32_t u32NumFifoEvents, const Pds::EvrData::FIFOEvent* lFifoEvent);
  EvrDataUtil(const EvrDataType& dataCopy);

  const Pds::EvrData::FIFOEvent& fifoEvent(unsigned i) const { return fifoEvents()[i]; }
  unsigned int  addFifoEvent        ( const Pds::EvrData::FIFOEvent& fifoEvent ); // return the number of total fifo events, including the new one  
  unsigned int  updateFifoEvent     ( const Pds::EvrData::FIFOEvent& fifoEvent ); // return the index to the updated fifo event
  int           updateFifoEventCheck( const Pds::EvrData::FIFOEvent& fifoEvent, unsigned int iMaxSize ); // return the index to the updated fifo event
  void          markEventAsDeleted  ( unsigned int iEventIndex );
  unsigned int  purgeDeletedEvents  (); // return the new total number of events after the purge
  

  unsigned int  removeTailEvent (); // return the number of total fifo events, after update
  void          clearFifoEvents ();
  void          printFifoEvents () const;

  unsigned size() const { return _sizeof(); }
  
  /*
   * public static function
   */  
  static unsigned int size(int iMaxNumFifoEvents);   
}; // class EvrDataUtil

} // namespace Pds

#endif // #ifndef Pds_EvrDataUtil_hh
