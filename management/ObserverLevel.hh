#ifndef PDS_ObserverLevel_HH
#define PDS_ObserverLevel_HH

#include "pds/utility/Stream.hh"
#include "CollectionObserver.hh"

namespace Pds {

  class EventCallback;
  class ObserverStreams;
  class EbIStream;
  class Allocation;
  class OutletWire;

  class ObserverLevel: public CollectionObserver {
  public:
    ObserverLevel(unsigned    platform,
		  const char* partition,
		  unsigned    node,
		  EventCallback& callback);
    virtual ~ObserverLevel();
  
    bool attach();
    void detach();

    void allocated(const Allocation&, unsigned);
    void dissolved();
  private:  
    void     post      (const Transition&);

  private:
    unsigned       _node;
    EventCallback& _callback;         // object to notify
    ObserverStreams * _streams;          // appliance streams
    EbIStream*     _inlet;
    OutletWire*    _outlets[StreamParams::NumberOfStreams];
};

}

#endif
