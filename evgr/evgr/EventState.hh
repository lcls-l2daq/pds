#ifndef Pds_EventState_hh
#define Pds_EventState_hh

#include "pds/config/EvrConfigType.hh"
#include "pds/evgr/EventCodeState.hh"
#include "pds/evgr/EvrDataUtil.hh"

namespace Pds {
  class FIFOEvent;
  class ClockTime;
  class EvrL1Data;

  class EventState {
  public:
    EventState();
    ~EventState();
  public:
    ///  clear configuration and state
    void clear();
    ///  clear state
    void clearFifoEvents();
    ///  configure
    void configure(const ndarray<const EventCodeType,1>&);
    void configure(const ndarray<const EvrData::SrcEventCode,1>&);
    ///  receive an eventcode
    void update   (const FIFOEvent&);
    ///  advance readout
    void advance  (EvrL1Data&, const ClockTime&, bool bIncomplete);
  private:
    enum { guNumTypeEventCode = 256 };
    enum { giMaxCommands      = 32 };
  public:
    /// Bit mask of readout groups to be generated
    uint32_t              uMaskReadout;
    /// List of commands to be generated
    unsigned              ncommands;
    char                  commands[giMaxCommands];
  private:
    ///  state of eventcodes to receive
    EventCodeState _codes[guNumTypeEventCode];
    ///  codes that contribute to coming L1Accepts
    ///  the next L1A.
    EvrDataUtil           _L1DataUpdated;
    ///  transient events whose delay has not expired.
    EvrDataUtil           _L1DataLatch;  
    /// transient events appearing a second time whose delay 
    /// for the first time has not expired.
    EvrDataUtil           _L1DataLatchQ;      
  };
};

#endif
