#ifndef PDS_RdmaObserverLevel_HH
#define PDS_RdmaObserverLevel_HH

#include "pds/utility/Stream.hh"
#include "CollectionObserver.hh"

namespace Pds {

  class EventCallback;
  class ObserverStreams;
  class WiredStreams;
  class Allocation;
  class OutletWire;

  class RdmaObserverLevel: public CollectionObserver {
  public:
    RdmaObserverLevel(unsigned       platform,
                      const char*    partition,
                      unsigned       nodes,
                      EventCallback& callback,
                      unsigned       max_eventsize = 0 // 0: default size
                      );
    virtual ~RdmaObserverLevel();

    bool attach();
    void detach();

    void allocated(const Allocation&);
    void dissolved();
  private:
    void     post      (const Transition&);
    void     post      (const InDatagram&);

  protected:
    WiredStreams* streams();

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
