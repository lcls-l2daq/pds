#ifndef Pds_EventCodeState_hh
#define Pds_EventCodeState_hh

#include "pds/config/EvrConfigType.hh"

namespace Pds {
  class FIFOEvent;
  class EventCodeState {
  public:
    EventCodeState();
    ~EventCodeState();
  public:
    void configure(const EventCodeType&);
    void configure(const EvrData::SrcEventCode&);
  public:
    uint32_t  uMaskReadout;
    bool      bCommand;
    bool      bSpecial;
    bool      bRelease;
    int       iDefReportDelay;
    int       iDefReportWidth;
    int       iReportWidth;
    int       iReportDelayQ; // First-order  delay for Control-Transient events
    int       iReportDelay;  // Second-order delay for Control-Transient events; First-order delay for Control-Latch events
  };
};

#endif
