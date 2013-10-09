#include "pds/client/L3FilterThreads.hh"
#include "pds/client/L3FilterDriver.hh"

#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/psddl/l3t.ddl.h"


#include "pds/xtc/InDatagram.hh"
#include "pds/service/Task.hh"

#include "pds/vmon/VmonServerManager.hh"
#include "pds/mon/MonCds.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntryTH1F.hh"
#include "pds/mon/MonDescTH1F.hh"

#include <math.h>
#include <signal.h>
#include <time.h>

#include <list>
#include <vector>
#include <exception>

//#define DBUG
//#define RECOVER_POOL

static const unsigned nbins = 64;
static const double ms_per_bin = 256./64.;

static double time_since(const timespec& now, const timespec& tv)
{
  double dt = double(now.tv_sec - tv.tv_sec)*1.e3;
  dt += (double(now.tv_nsec)-double(tv.tv_nsec))*1.e-6;
  return dt;
}

namespace Pds {
  namespace L3F {
    class Entry {
    public:
      Entry(Transition* tr) : _state(Completed), _type(TypeT), _ptr(tr)
      { clock_gettime(CLOCK_REALTIME,&_start); }
      Entry(InDatagram* in) : 
        _state(in->datagram().seq.service()==TransitionId::L1Accept ||
               in->datagram().seq.service()==TransitionId::Configure ? 
               Queued : Completed),
	_type(TypeI), _ptr(in)
      { clock_gettime(CLOCK_REALTIME,&_start); }
    public:
      bool is_complete  () const { return _state==Completed; }
      bool is_unassigned() const { return _state==Queued; }
    public:
      void assign  () { _state=Assigned; }
      void complete() { _state=Completed; }
      void post    (Pds::Appliance& app) { 
        if (_type == TypeT) app.post((Transition*)_ptr);
        else                app.post((InDatagram*)_ptr);
      }
      void* ptr() { return _ptr; }
    public:
      double since_start (const timespec& now) const { return time_since(now,_start); }
    private:
      enum { Queued, Assigned, Completed } _state;
      enum { TypeT, TypeI } _type;
      void* _ptr;
      timespec _start;
    };

    class Manager;

    class QueueTr : public Routine {
    public:
      QueueTr(Transition* tr, Manager& app) : _tr(tr), _app(app) {}
      void routine();
    private:
      Transition*   _tr;
      Manager&      _app;
    };

    class QueueEv : public Routine {
    public:
      QueueEv(InDatagram* in, Manager& app) : _in(in), _app(app) {}
      void routine();
    private:
      InDatagram*   _in;
      Manager&      _app;
    };

    class ComplEv : public Routine {
    public:
      ComplEv(L3F::Entry* in,L3F::Manager& app,unsigned id) : _in(in), _app(app), _id(id) {}
      void routine();
    private:
      L3F::Entry*   _in;
      L3F::Manager& _app;
      unsigned      _id;
    };
    
    class Task : public Routine {
    public:
      Task(unsigned id, L3F::Manager& app, L3FilterDriver* driver) :
        _id(id), _app(app), _driver(driver),
        _task(new Pds::Task(TaskObject("L3Ftsk"))),
        _entry(0) {}
      ~Task() { _task->destroy(); delete _driver; }
    public:
      void assign(L3F::Entry* e) { (_entry = e)->assign(); _task->call(this); }
      void unassign() { _entry = 0; }
      bool unassigned() const { return _entry==0; }
      void routine();
      void process(InDatagram* dg) { _driver->events(dg); }
      unsigned id() const { return _id; }
      const L3F::Entry* entry() const { return _entry; }
      void interrupt();
    private:
      unsigned        _id;
      L3F::Manager&   _app;
      L3FilterDriver* _driver;
      Pds::Task*      _task;
      L3F::Entry*     _entry;
    };

    class Manager {
    public:
      Manager(Appliance& app,
              Pds::Task& mgr_task) : 
        _app     (app),
        _mgr_task(mgr_task)
      {
        MonGroup* group = new MonGroup("L3F");
        VmonServerManager::instance()->cds().add(group);
        
        MonDescTH1F start_to_complete("Start to Complete","[ms]", "", nbins, 0., double(nbins)*ms_per_bin);
        _start_to_complete = new MonEntryTH1F(start_to_complete);
        group->add(_start_to_complete);

        MonDescTH1F start_to_complete_log("Log Start to Complete","log10 [ms]", "", 
					  nbins, 1., 4.);
        _start_to_complete_log = new MonEntryTH1F(start_to_complete_log);
        group->add(_start_to_complete_log);
      }
      ~Manager() {
        for(unsigned id=0; id<_tasks.size(); id++)
          delete _tasks[id];
      }
    public:
      void tasks(const std::vector<Task*>& tasks) { _tasks=tasks; }
      Pds::Task& mgr_task() { return _mgr_task; }
      void queueTransition(Transition* tr)
      {
        _list.push_back(new Entry(tr));
        _process();
      }
      void queueEvent     (InDatagram* in)
      {
	//  Configure needs to go to every instance
	if (in->datagram().seq.service()==TransitionId::Configure) {
	  unsigned extent = in->datagram().xtc.extent;
	  for(unsigned id=1; id<_tasks.size(); id++) {
	    _tasks[id]->process(in);
	    in->datagram().xtc.extent = extent;  // remove any insertions
	  }
	  Entry* e = new Entry(in);
	  _list.push_back(e);
	  _tasks[0]->assign(e);
	}
	else {
	  _list.push_back(new Entry(in));
	}
	_process();
      }
      void completeEntry  (Entry* e, unsigned id)
      {
        _complete(e);
        _tasks[id]->unassign();
        _process();
      }
    private:
      void _process()
      {
        //  First post the entries in order that are complete
        while( !_list.empty() ) {
          Entry* e=_list.front();
          if (!e->is_complete()) {
	    //  If the datagram pool is empty, force completion
#ifdef RECOVER_POOL
#ifdef DBUG
	    printf("L3FilterDriver entry still queued with pool depth %d\n",
		   Pool::numberOfFreeObjects(e->ptr()));
#endif
	    if (Pool::numberOfFreeObjects(e->ptr()) < 1) {
	      printf("L3FilterDriver input pool is (near) empty\n");
	      for(unsigned id=0; id<_tasks.size(); id++)
		if (_tasks[id]->entry() == e) {
		  printf("L3FilterDriver interrupting task %d\n",id);
		  _tasks[id]->interrupt();
		  break;
		}
	    }
#endif
	    break;
	  }

          _list.pop_front();
          e->post(_app);
	  delete e;
        }

        //  Next assign entries in order that haven't yet been
        for(std::list<Entry*>::iterator it=_list.begin(); 
            it!=_list.end(); it++) {
          if ((*it)->is_unassigned()) {
            bool lassign=false;
            for(unsigned id=0; id<_tasks.size(); id++)
              if (_tasks[id]->unassigned()) {
                _tasks[id]->assign(*it);
                lassign=true;
#ifdef DBUG
		printf("L3FilterThreads assign to thread %d\n",id);
#endif
                break;
              }
            //  Too busy to process now
            if (!lassign) {
#ifdef DBUG
	      printf("L3FilterThreads unassigned\n");
#endif
	      _complete(*it);
	    }
          }
        }
      }
      void _complete(Entry* e)
      {
        timespec now;
        clock_gettime(CLOCK_REALTIME,&now);
        ClockTime time(now.tv_sec,now.tv_nsec);
	double dt = e->since_start(now);
	_start_to_complete->addcontent(1.,dt);
	_start_to_complete->time(time);
	_start_to_complete_log->addcontent(1.,log10f(dt));
	_start_to_complete_log->time(time);

        e->complete();
      }
    private:
      Pds::Appliance&           _app;
      std::vector<L3F::Task*>   _tasks;
      Pds::Task&                _mgr_task;
      std::list  <L3F::Entry*>  _list;
      MonEntryTH1F*             _start_to_complete;
      MonEntryTH1F*             _start_to_complete_log;
    };
  };
};

using namespace Pds;

void L3F::Task::routine() {
  _driver->events((InDatagram*)_entry->ptr());
  _app.mgr_task().call(new L3F::ComplEv(_entry,_app,_id));
}

class sigusr_exception : public std::exception {
public:
  sigusr_exception() {}
public:
  const char* what() const throw() { return "sigusr_exception"; }
};

static void sigusr_handler(int signum)
{
  throw sigusr_exception();
}

void L3F::Task::interrupt() {
  _task->signal(SIGUSR1);
}

void L3F::QueueTr::routine() { _app.queueTransition(_tr); delete this; }
void L3F::QueueEv::routine() { _app.queueEvent(_in); delete this; }
void L3F::ComplEv::routine() { _app.completeEntry(_in,_id); delete this; }

L3FilterThreads::L3FilterThreads(create_m* c_user, unsigned nthreads) :
  _mgr_task(new Task(TaskObject("L3Fmgr")))
{
  _mgr = new L3F::Manager(*this, *_mgr_task);
  
  if (!nthreads) nthreads=4;

  std::vector<L3F::Task*> tasks(nthreads);
  for(unsigned id=0; id<nthreads; id++)
    tasks[id] = new L3F::Task(id,*_mgr,
                              new L3FilterDriver(c_user()));
#ifdef DBUG
  printf("L3FilterThreads making %d [%d] threads\n",nthreads,tasks.size());
#endif

  _mgr->tasks(tasks);

#ifdef RECOVER_POOL
  struct sigaction act;
  act.sa_handler  = &sigusr_handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags    = 0;
  if (sigaction(SIGUSR1, &act, NULL) < 0)
    perror("L3FilterThreads failed to install SIGUSR1 handler");
#endif
}

L3FilterThreads::~L3FilterThreads()
{
  _mgr_task->destroy();
  delete _mgr;
}

Transition* L3FilterThreads::transitions(Transition* tr)
{
  _mgr_task->call(new L3F::QueueTr(tr,*_mgr));
  return (Transition*)Appliance::DontDelete;
}

InDatagram* L3FilterThreads::events(InDatagram* in)
{
  _mgr_task->call(new L3F::QueueEv(in,*_mgr));
  return (InDatagram*)Appliance::DontDelete;
}
