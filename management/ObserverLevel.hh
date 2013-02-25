#ifndef PDS_ObserverLevel_HH
#define PDS_ObserverLevel_HH

#include "pds/utility/Stream.hh"
#include "CollectionObserver.hh"

namespace Pds {

  class EventCallback;
  class ObserverStreams;
  class Allocation;
  class OutletWire;

  class ObserverLevel: public CollectionObserver {
  public:
    ObserverLevel(unsigned    platform,
      const char* partition,
      unsigned    nodes,
      EventCallback& callback,
      int            slowEb,
      unsigned       max_eventsize = 0 // 0: default size
      );
    virtual ~ObserverLevel();

    bool attach();
    void detach();

    void allocated(const Allocation&);
    void dissolved();
  private:
    void     post      (const Transition&);
    void     post      (const InDatagram&);

  private:
    unsigned          _nodes;
    EventCallback&    _callback;         // object to notify
    ObserverStreams * _streams;          // appliance streams
    int               _slowEb;
    unsigned          _max_eventsize;
    OutletWire*       _outlets[StreamParams::NumberOfStreams];
};

}

#endif
