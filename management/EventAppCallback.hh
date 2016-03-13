#ifndef PDS_EVENTAPPCALLBACK_HH
#define PDS_EVENTAPPCALLBACK_HH

#include "pds/management/EventCallback.hh"

#include <list>

namespace Pds {

  class Appliance;
  class Node;
  class SetOfStreams;
  class Task;

  class EventAppCallback : public EventCallback {
  public:
    EventAppCallback(Task*,
                     unsigned platform,
                     Appliance&);
    EventAppCallback(Task*,
                     unsigned platform,
                     std::list<Appliance*>);
    ~EventAppCallback();
    
    void attached (SetOfStreams& streams);
    void failed   (Reason reason);
    void dissolved(const Node& who);
  private:
    Task*                 _task;
    unsigned              _platform;
    std::list<Appliance*> _app;
  };

}
#endif
