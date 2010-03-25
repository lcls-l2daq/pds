#include "pds/config/EvrConfigType.hh" // typedefs for the Evr config data types

namespace Pds
{
  
class EvrDataUtil : public EvrDataType
{  
public:
  /* public functions*/  
  EvrDataUtil(uint32_t u32NumFifoEvents, const EvrData::FIFOEvent* lFifoEvent);
  EvrDataUtil(const EvrDataType& dataCopy);

  void          printFifoEvents () const;
  unsigned int  addFifoEvent    ( const EvrData::FIFOEvent& fifoEvent ); // return the number of total fifo events, including the new one
  void          clearFifoEvents ();
  
  using EvrDataType::size; // unhide the size() member function
  
  /*
   * public static function
   */  
  static unsigned int size(int iMaxNumFifoEvents);   
}; // class EvrDataUtil

} // namespace Pds
