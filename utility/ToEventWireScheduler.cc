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

#include <string.h>
#include <errno.h>
#include <poll.h>

using namespace Pds;

static unsigned _maxscheduled = 4;
static int      _idol_timeout  = 250; // idol time [ms] which forces flush of queued events
static int      _disable_buffer = 10; // time [ms] inserted between flushed L1 and Disable transition

void ToEventWireScheduler::setMaximum(unsigned m) { _maxscheduled = m; }

ToEventWireScheduler::ToEventWireScheduler(Outlet& outlet,
             CollectionManager& collection,
             int interface,
             int maxbuf,
             const Ins& occurrences) :
  OutletWire   (outlet),
  _collection  (collection),
  _client      (sizeof(OutletWireHeader), Mtu::Size, Ins(interface),
    1 + maxbuf / Mtu::Size),
  _occurrences (occurrences),
  _scheduled   (0),
  _task        (new Task(TaskObject("TxScheduler")))
{
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
  _collection.ucast(*tr,_occurrences);
  return 0;
}

void ToEventWireScheduler::_flush(InDatagram* dg)
{
  TrafficDst* n = dg->traffic(_bcast);
  while (n->send_next(_client)) 
  {
#ifdef BUILD_PACKAGE_SPACE
    const struct timespec timeSleep = { 0, 1000000 };
    nanosleep(&timeSleep, NULL);
#endif
  }
  delete n;
}

void ToEventWireScheduler::_flush()
{
  TrafficDst* t = _list.forward();
  while(t != _list.empty()) {
    do {
      TrafficDst* n = t->forward();

#ifdef BUILD_PACKAGE_SPACE
      const struct timespec timeSleep = { 0, 1000000 };
      nanosleep(&timeSleep, NULL);
#endif

      if (!t->send_next(_client))
  delete t->disconnect();
      t = n;
    } while( t != _list.empty());
    t = _list.forward();
  }
  _scheduled  = 0;
  _nscheduled = 0;
}

InDatagram* ToEventWireScheduler::forward(InDatagram* dg)
{
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
  //    [missing a timeout?]
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
    if (_nscheduled) {
      _flush();
      //
      //  Add some spacing to insure that the L1's and Transitions are not "coalesced"
      //
      timespec ts;
      ts.tv_sec = 0; ts.tv_nsec = _disable_buffer*1000000;
      nanosleep(&ts,0);
    }
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
