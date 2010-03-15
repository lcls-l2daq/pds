#ifndef Pds_ToEventWireScheduler_hh
#define Pds_ToEventWireScheduler_hh

#include <sys/uio.h>

#include "pds/utility/OutletWire.hh"
#include "pds/service/Routine.hh"

#include "pds/utility/OutletWireInsList.hh"
#include "pds/service/Client.hh"

namespace Pds {
  class CollectionManager;
  class Outlet;
  class Datagram;
  class Task;
  class TrafficDst;
  class TrafficScheduler;

  class ToEventWireScheduler : public OutletWire,
			       public Routine {
  public:
    ToEventWireScheduler(Outlet& outlet,
			 CollectionManager&,
			 int interface,
			 int maxbuf,
			 const Ins& occurrences,
			 int maxscheduled = 4,    // traffic shaping width
			 int idol_timeout = 250); // idol time [ms] which forces flush of queued events
    ~ToEventWireScheduler();
  public:  
    // ToEventWire interface
    virtual Transition* forward(Transition* dg);
    virtual Occurrence* forward(Occurrence* dg);
    virtual InDatagram* forward(InDatagram* dg);
    virtual void bind(NamedConnection, const Ins& );
    virtual void bind(unsigned id, const Ins& node);
    virtual void unbind(unsigned id);
    // Routine interface
    void routine();
  private:
    void _flush(InDatagram*);
    void _flush();
  private:
    OutletWireInsList  _nodes;
    CollectionManager& _collection;
    Client             _client;
    Ins                _bcast;
    const Ins&         _occurrences;
    int                    _maxscheduled;
    int                    _nscheduled;
    unsigned               _scheduled;
    int                    _idol_timeout;
    LinkedList<TrafficDst> _list;
    TrafficScheduler*      _schedule;
    Task*                  _task;
    int                    _schedfd[2];
  };
}

#endif
