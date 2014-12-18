#include "ToEventWireScheduler.hh"

#include "pds/utility/Appliance.hh"
#include "pds/utility/Mtu.hh"
#include "pds/utility/Transition.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/OutletWireHeader.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/TrafficDst.hh"
#include "pds/collection/CollectionManager.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/service/Task.hh"

#include "pds/vmon/VmonServerManager.hh"
#include "pds/mon/MonCds.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntryTH1F.hh"
#include "pds/mon/MonDescTH1F.hh"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>

static unsigned _maxscheduled = 4;

static unsigned tbin_shift = 10; // 1<<tbin_shift microseconds/bin
static unsigned tbin_range = 16; // 1<<tbin_range microseconds full range

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
      timespec start, end;
      if (_scheduler)
        clock_gettime(CLOCK_REALTIME, &start);

      unsigned cnt=_maxscheduled;
      TrafficDst* t = _list.forward();
      while(t != _list.empty()) {
        //  Fill in
        while(cnt < _maxscheduled) {
          t->send_copy(_client,StreamPorts::sink());
          cnt++;
        }
        cnt = 0;
        do {
          cnt++;
          TrafficDst* n = t->forward();

          if (!t->send_next(_client))
            delete t->disconnect();
          t = n;
        } while( t != _list.empty());
        t = _list.forward();
      }

      if (_scheduler) {
        clock_gettime(CLOCK_REALTIME, &end);
        _scheduler->histo(start,end);
      }

      delete this;
    }
  private:
    LinkedList<TrafficDst>  _list;
    Client&                 _client;
    ToEventWireScheduler*   _scheduler;
  };
};

using namespace Pds;

static int      _idol_timeout  = 150;  // idol time [ms] which forces flush of queued events.

static int      _disable_buffer = 100; // time [ms] inserted between flushed L1 and Disable transition
static unsigned _phase = 0;
static unsigned _interval = 8000;  // time interval [us] for traffic shaping

static bool _shape_tmo = false;

void ToEventWireScheduler::setMaximum (unsigned m) { _maxscheduled = m; }
void ToEventWireScheduler::setPhase   (unsigned m) { _phase = m; }
void ToEventWireScheduler::setInterval(unsigned m) { _interval = m; }
void ToEventWireScheduler::shapeTmo   (bool v) { _shape_tmo = v; }

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
  _nscheduled  (0),
  _scheduled   (0),
  _task        (new Task(TaskObject("TxScheduler"))),
  _flush_task  (new Task(TaskObject("TxFlush"))),
  _routineq    (0)
{
  _flushCount = 0;

  MonGroup* group = new MonGroup("ToEvent");
  VmonServerManager::instance()->cds().add(group);

  MonDescTH1F sendtime("Send Time", "[us]", "", 
                       1<<(tbin_range-tbin_shift), 0., float(1<<tbin_range));
  _histo = new MonEntryTH1F(sendtime);
  group->add(_histo);

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
    _queue();
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
  if (_routineq) {
    _flush_task->call(_routineq);
    _routineq = 0;
  }
}

void ToEventWireScheduler::_queue()
{
  if (_nscheduled==0)
    return;

  //
  //  Phase delay goes here
  //
  timeval timeSleepMicro = {0, _phase*_interval};
  select( 0, NULL, NULL, NULL, &timeSleepMicro);

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

          if ((m & _scheduled) || (_shape_tmo && (_nscheduled>=_maxscheduled))) {
            _queue();
	    if (_nscheduled == _phase)
	      _flush();
	  }

          TrafficDst* t = dg->traffic(dst->ins());
          _list.insert(t);
          _scheduled |= m;
          ++_nscheduled;

          if (!_shape_tmo && _nscheduled >= _maxscheduled)
            _queue();

	  if (_nscheduled == _phase)
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
#if 0
      if (_shape_tmo && (_list.forward() != _list.empty())) {
        //
        //  Workaround to shape transmission of an additional event (clone of an existing one and
        //  destined for an unused multicast address) when we run at low rates (< 1/idol_timeout [6Hz]).
        //  This is needed to prevent drops, for reasons I don't understand, when traffic shaping is absent.  
        //  The additional transmission only works if the additional contribution can be transmitted
        //  during the idol_timeout interval; i.e. 2 events * eth_rate < idol_timeout 
        //  [ for eth_rate = 1GB/s, idol_timeout = 150ms => event_size < 37.5 MB ]
        //
        LinkedList<TrafficDst> list;
        list.insert( _list.forward()->clone() );
        list.insertList(&_list);
        _list.insertList(&list);
      }
#endif
      _queue();
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
  double diff = double(end.tv_sec - start.tv_sec);
  diff += double(end.tv_nsec)*1.e-9;
  diff -= double(start.tv_nsec)*1.e-9;
  unsigned udiff = unsigned(diff*1.e6);
  if (udiff >> tbin_range)
    _histo->addinfo(1.,MonEntryTH1F::Overflow);
  else
    _histo->addcontent(1.,udiff >> tbin_shift);

  _histo ->time(ClockTime(end.tv_sec,end.tv_nsec));
}
