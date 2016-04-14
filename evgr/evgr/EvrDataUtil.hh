#ifndef Pds_EvrDataUtil_hh
#define Pds_EvrDataUtil_hh

#include "pds/config/EvrConfigType.hh" // typedefs for the Evr config data types
#include "evgr/evr/evr.hh"

#include <list>

namespace Pds
{
  class EventCodeState;

  class EvrDataUtil
  {  
  public:
    /* public functions*/  
    EvrDataUtil();
    //    EvrDataUtil(uint32_t u32NumFifoEvents, const EvrData::FIFOEvent* lFifoEvent);
    //    EvrDataUtil(const EvrDataType& dataCopy);

    //    const EvrData::FIFOEvent& fifoEvent(unsigned i) const { return fifoEvents()[i]; }
    ///  Add the event to the cache
    void          addFifoEvent        ( const FIFOEvent& fifoEvent ); 
    ///  Add the special event removing all expired specials
    void          addSpecialEvent     ( const FIFOEvent& fifoEvent ); 
    ///  Update the time if the code exists, else add the code
    void          updateFifoEvent     ( const FIFOEvent& fifoEvent ); 
    ///  Remove latch codes that match the release code
    void          checkLatch          ( const FIFOEvent& fifoEvent,
					const EventCodeState* codes );
    ///  Advance timers; if expired, move to next cache
    void          advanceQ            ( EvrDataUtil&    next,
					EventCodeState* codes );
    void          advance             ( EvrDataUtil&    next,
					EventCodeState* codes );
    void          insert              ( const EvrDataUtil&    next );

    void          clearFifoEvents ();
    void          printFifoEvents () const;
						
    ///  Data was truncated at some point
    bool          full() const { return _full; }

    EvrDataType*  write(void*);

  private:
    unsigned numFifoEvents() const;

  private:
    class FifoEventT {
    public:
      FifoEventT() {}
      FifoEventT(const FIFOEvent& e, bool t) :
	event    (e),
	transient(t) {}
    public:
      FIFOEvent event;
      bool      transient;
    };
    std::list<FifoEventT> _data;
    bool                  _full;
  };

} // namespace Pds

#endif // #ifndef Pds_EvrDataUtil_hh
