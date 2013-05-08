#ifndef Pds_ToEventWireScheduler_hh
#define Pds_ToEventWireScheduler_hh

#include <sys/uio.h>

#include "pds/utility/OutletWire.hh"
#include "pds/service/Routine.hh"

#include "pds/utility/OutletWireInsList.hh"
#include "pds/service/Client.hh"
#include <time.h>

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
			 const Ins& occurrences);
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
  public:
    static void setMaximum(unsigned);
    static void setPhase  (unsigned);
    static void shapeTmo  (bool);
  private:
    void _queue(TrafficDst*);
    void _flush(InDatagram*);
    void _flush();
  public:
    void histo(timespec&, timespec&);
  private:
    OutletWireInsList  _nodes;
    CollectionManager& _collection;
    Client             _client;
    Ins                _bcast;
    const Ins&         _occurrences;
    unsigned               _nscheduled;
    unsigned               _scheduled;
    LinkedList<TrafficDst> _list;
    TrafficScheduler*      _schedule;
    Task*                  _task;
    Task*                  _flush_task;
    int                    _schedfd[2];
    unsigned               _flushCount;
    unsigned*              _histo;
  };
}

#endif
