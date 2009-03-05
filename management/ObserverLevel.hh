#ifndef PDS_ObserverLevel_HH
#define PDS_ObserverLevel_HH

#include "CollectionObserver.hh"

namespace Pds {

  class EventCallback;
  class EventStreams;
  class EbIStream;
  class Allocate;

  class ObserverLevel: public CollectionObserver {
  public:
    ObserverLevel(unsigned    platform,
		  const char* partition,
		  unsigned    node,
		  EventCallback& callback);
    virtual ~ObserverLevel();
  
    bool attach();
    void detach();

    void allocated(const Allocate&, unsigned);
    void dissolved();
  private:  
    void     post      (const Transition&);

  private:
    unsigned       _node;
    EventCallback& _callback;         // object to notify
    EventStreams*  _streams;          // appliance streams
    EbIStream*     _inlet;
  };

}

#endif
