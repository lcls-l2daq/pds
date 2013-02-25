#include "ToEventWireScheduler.hh"

#include "pds/utility/Appliance.hh"
#include "pds/utility/Mtu.hh"
#include "pds/utility/Transition.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/OutletWireHeader.hh"
#include "pds/utility/TrafficDst.hh"
#include "pds/collection/CollectionManager.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/service/Task.hh"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>

namespace Pds {
  class FlushRoutine : public Routine {
  public:
    FlushRoutine(LinkedList<TrafficDst>& list,
     Client&                 client,
     ToEventWireScheduler*   scheduler=0) :
      _client   (client),
      _scheduler(scheduler)
    {
      _list.insertList(&list);
    }
    ~FlushRoutine() {} // _list should be empty
  public:
    void routine()
    {
#ifdef HISTO_TRANSMIT_TIMES
      timespec start, end;
      if (_scheduler)
  clock_gettime(CLOCK_REALTIME, &start);
#endif
      TrafficDst* t = _list.forward();
      while(t != _list.empty()) {
  do {
    TrafficDst* n = t->forward();

//#ifdef BUILD_PACKAGE_SPACE
//    static int iDgCount = 0;
//    if (++iDgCount > 100)
//      {
//        timeval timeSleepMicro = {0, 1000};
//        // Use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
//        select( 0, NULL, NULL, NULL, &timeSleepMicro);
//        iDgCount = 0;
//      }
//#endif

    if (!t->send_next(_client))
      delete t->disconnect();
    t = n;
  } while( t != _list.empty());
  t = _list.forward();
      }
#ifdef HISTO_TRANSMIT_TIMES
      if (_scheduler) {
  clock_gettime(CLOCK_REALTIME, &end);
  _scheduler->histo(start,end);
      }
#endif

      delete this;
    }
  private:
    LinkedList<TrafficDst>  _list;
    Client&                 _client;
    ToEventWireScheduler*   _scheduler;
  };
};

using namespace Pds;

static unsigned _maxscheduled = 4;

//#ifdef BUILD_PRINCETON
//static int      _idol_timeout  = 150; // idol time [ms] which forces flush of queued events
//#else
//static int      _idol_timeout  = 150;  // idol time [ms] which forces flush of queued events, to be sync at 0.5Hz
//#endif
static int      _idol_timeout  = 150;  // idol time [ms] which forces flush of queued events

static int      _disable_buffer = 100; // time [ms] inserted between flushed L1 and Disable transition
static unsigned _phase = 0;

static long long int timeDiff(timespec* end, timespec* start) {
  long long int diff;
  diff =  (end->tv_sec - start->tv_sec) * 1000000000LL;
  diff += end->tv_nsec;
  diff -= start->tv_nsec;
  return diff;
}

void ToEventWireScheduler::setMaximum(unsigned m) { _maxscheduled = m; }
void ToEventWireScheduler::setPhase  (unsigned m) { _phase = m; }

ToEventWireScheduler::ToEventWireScheduler(Outlet& outlet,
             CollectionManager& collection,
             int interface,
             int maxbuf,
             const Ins& occurrences) :
  OutletWire   (outlet),
  _collection  (collection),
  _client      (sizeof(OutletWireHeader), Mtu::Size, Ins(interface),
                //                (1 + maxbuf / Mtu::Size)*_maxscheduled),
                ((1 + maxbuf) / Mtu::Size)),
  _occurrences (occurrences),
  _task        (new Task(TaskObject("TxScheduler"))),
  _flush_task  (new Task(TaskObject("TxFlush")))
{
  _flushCount = 0;
  _histo = (unsigned*) calloc(10000, sizeof(unsigned));
  if (::pipe(_schedfd) < 0)
    printf("ToEventWireScheduler pipe open error : %s\n",strerror(errno));
  else
    _task->call(this);
}

ToEventWireScheduler::~ToEventWireScheduler()
{
  ::close(_schedfd[0]);
  ::close(_schedfd[1]);
  _task->destroy();
}

Transition* ToEventWireScheduler::forward(Transition* tr)
{
  // consider emptying event queues first
  Ins dst(tr->reply_to());
  _collection.ucast(*tr,dst);
  return 0;
}

Occurrence* ToEventWireScheduler::forward(Occurrence* tr)
{
  if (tr->id() != OccurrenceId::EvrCommand)
    _collection.ucast(*tr,_occurrences);
  return 0;
}

void ToEventWireScheduler::_flush(InDatagram* dg)
{
  if (_nscheduled) {
    _flush();
    //
    //  Add some spacing to insure that the L1's and Transitions are not "coalesced"
    //
    timespec ts;
    ts.tv_sec = 0; ts.tv_nsec = _disable_buffer*1000000;
    nanosleep(&ts,0);
  }

  TrafficDst* n = dg->traffic(_bcast);
  _list.insert(n);

  _flush_task->call( new FlushRoutine(_list,_client) );
}

void ToEventWireScheduler::_flush()
{
  if (_nscheduled==0)
    return;

  //
  //  Phase delay goes here
  //
  timeval timeSleepMicro = {0, _phase*8000}; // 8 milliseconds
  select( 0, NULL, NULL, NULL, &timeSleepMicro);

  _flush_task->call( new FlushRoutine(_list,_client,this) );

  _scheduled  = 0;
  _nscheduled = 0;
}

InDatagram* ToEventWireScheduler::forward(InDatagram* dg)
{
//  if (dg->datagram().seq.isEvent())
//    return 0;

  ::write(_schedfd[1], &dg, sizeof(dg));
  return (InDatagram*)Appliance::DontDelete;
}

void ToEventWireScheduler::routine()
{
  pollfd pfd;
  pfd.fd      = _schedfd[0];
  pfd.events  = POLLIN | POLLERR;
  pfd.revents = 0;
  int nfd = 1;
  while(1) {
    if (::poll(&pfd, nfd, _idol_timeout) > 0) {
      InDatagram* dg;
      if (::read(_schedfd[0], &dg, sizeof(dg)) == sizeof(dg)) {
  //  Flush the set of events if
  //    (1) we already have queued an event to the same destination
  //    (2) we have reached the maximum number of queued events
  const Sequence& seq = dg->datagram().seq;
  if (seq.isEvent() && !_nodes.isempty()) {
    OutletWireIns* dst = _nodes.lookup(seq.stamp().vector());
    unsigned m = 1<<dst->id();
    if (m & _scheduled)
      _flush();
    TrafficDst* t = dg->traffic(dst->ins());
    _list.insert(t);
    _scheduled |= m;
    if (++_nscheduled >= _maxscheduled)
      _flush();
  }
  else {
    _flush(dg);
  }
      }
      else {
  printf("ToEventWireScheduler::routine error reading pipe : %s\n",
         strerror(errno));
  break;
      }
    }
    else {  // timeout
      _flush();
    }
  }
}

void ToEventWireScheduler::bind(NamedConnection, const Ins& ins)
{
  _bcast = ins;
}

void ToEventWireScheduler::bind(unsigned id, const Ins& node)
{
  _nodes.insert(id, node);
}

void ToEventWireScheduler::unbind(unsigned id)
{
  _nodes.remove(id);
}

void ToEventWireScheduler::histo(timespec& start, timespec& end)
{
  long long int interval = timeDiff(&end, &start)/100000LL;
  if (interval > 9999) interval = 9999;
  _histo[interval] += 1;
  if ((++_flushCount % 10000) == 0) {
    printf("ToEventWireScheduler time histo in ms %u\n", _flushCount);
    for (int i=0; i<1000; i++) {
      if (_histo[i]) {
        printf("\t%5.1f - %8u\n", i/10.0, _histo[i]);
      }
    }
  }
}
