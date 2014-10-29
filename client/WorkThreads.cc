#include "pds/client/WorkThreads.hh"

#include "pdsdata/xtc/XtcIterator.hh"

#include "pds/xtc/InDatagram.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"
#include "pds/service/GenericPoolW.hh"
#include "pds/service/Semaphore.hh"

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
#include <string>

//#define DBUG

static const unsigned nbins = 64;
static const double ms_per_bin = 256./64.;

static double time_since(const timespec& now, const timespec& tv)
{
  double dt = double(now.tv_sec - tv.tv_sec)*1.e3;
  dt += (double(now.tv_nsec)-double(tv.tv_nsec))*1.e-6;
  return dt;
}

namespace Pds {
  namespace Work {
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
      ComplEv(Work::Entry* in,Work::Manager& app,unsigned id) : _in(in), _app(app), _id(id) {}
      void routine();
    private:
      Work::Entry*   _in;
      Work::Manager& _app;
      unsigned      _id;
    };
    
    class Task : public Routine {
    public:
      Task(unsigned id, Work::Manager& app, Appliance* driver) :
        _id(id), _app(app), _driver(driver),
        _task(new Pds::Task(TaskObject("L3Ftsk"))),
        _entry(0) {}
      ~Task() { _task->destroy(); delete _driver; }
    public:
      void assign(Work::Entry* e) { (_entry = e)->assign(); _task->call(this); }
      void unassign() { _entry = 0; }
      bool unassigned() const { return _entry==0; }
      void routine();
      void process(InDatagram* dg) { _driver->events(dg); }
      void process(Transition* tr) { _driver->transitions(tr); }
      unsigned id() const { return _id; }
      const Work::Entry* entry() const { return _entry; }
      void interrupt();
    private:
      unsigned         _id;
      Work::Manager&   _app;
      Appliance*       _driver;
      Pds::Task*       _task;
      Work::Entry*     _entry;
    };

    class Manager {
    public:
      Manager(const char* name,
              Appliance& app,
              Pds::Task& mgr_task) : 
        _app     (app),
        _mgr_task(mgr_task),
	_pool    (sizeof(UserMessage),1),
	_sem     (Semaphore::FULL),
	_handled (false)
      {
        MonGroup* group = new MonGroup(name);
        VmonServerManager::instance()->cds().add(group);
        
        MonDescTH1F start_to_complete("Start to Complete","[ms]", "", nbins, 0., double(nbins)*ms_per_bin);
        _start_to_complete = new MonEntryTH1F(start_to_complete);
        group->add(_start_to_complete);

        MonDescTH1F queued("Queued"      ,"[events]","", 32, -0.5, 31.5);
        _queued    = new MonEntryTH1F(queued);
        group->add(_queued);

        MonDescTH1F assigned("Assigned"  ,"[events]","", 32, -0.5, 31.5);
        _assigned  = new MonEntryTH1F(assigned);
        group->add(_assigned);

        MonDescTH1F completed("Completed","[events]","", 32, -0.5, 31.5);
        _completed = new MonEntryTH1F(completed);
        group->add(_completed);
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
	_handled=false;

	try {
	  for(unsigned id=0; id<_tasks.size(); id++)
	    _tasks[id]->process(tr);
	}
	catch (std::exception& e) {
	  printf("WorkThreads::Task::queueTransition caught %s\n",e.what());
	  handle(e.what());
	}
	catch (std::string& e) {
	  printf("WorkThreads::Task::queueTransition caught %s\n",e.c_str());
	  handle(e.c_str());
	}
	catch (...) {
	  printf("WorkThreads::Task::queueTransition caught unknown exception\n");
	  handle("Unknown plugin exception");
	}

        _app.post(tr);
      }
      void queueEvent     (InDatagram* in)
      {
	//  nonL1 transitions need to go to every instance
	if (in->datagram().seq.service()!=TransitionId::L1Accept) {
	  unsigned extent = in->datagram().xtc.extent;
	  try {
	    for(unsigned id=1; id<_tasks.size(); id++) {
	      _tasks[id]->process(in);
	      in->datagram().xtc.extent = extent;  // remove any insertions
	    }
	    _tasks[0]->process(in);
	  }
	  catch (std::exception& e) {
	    printf("WorkThreads::Task::queueEvent caught %s\n",e.what());
	    handle(e.what());
	  }
	  catch (std::string& e) {
	    printf("WorkThreads::Task::queueEvent caught %s\n",e.c_str());
	    handle(e.c_str());
	  }
	  catch (...) {
	    printf("WorkThreads::Task::queueEvent caught unknown exception\n");
	    handle("Unknown plugin exception");
	  }

          _app.post(in);
	}
	else {
          _audit();
	  _list.push_back(new Entry(in));
          _process();
	}
      }
      void completeEntry  (Entry* e, unsigned id)
      {
        _complete(e);
        _tasks[id]->unassign();
        _process();
      }
      void handle (const char* smsg)
      {
	_sem.take();
	if (!_handled) {
	  _handled=true;
	  _app.post(new (&_pool) Occurrence (OccurrenceId::ClearReadout));
	  _app.post(new (&_pool) UserMessage(smsg));
	}
	_sem.give();
      }
    private:
      void _process()
      {
        //  First post the entries in order that are complete
        while( !_list.empty() ) {
          Entry* e=_list.front();
          if (!e->is_complete()) {
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
		printf("WorkThreads assign to thread %d\n",id);
#endif
                break;
              }
            //  Too busy to process now
            if (!lassign) {
#ifdef DBUG
	      printf("WorkThreads unassigned\n");
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
        e->complete();
      }
      void _audit()
      {
        unsigned assigned=0, completed=0;
        //  Next assign entries in order that haven't yet been
        for(std::list<Entry*>::iterator it=_list.begin(); it!=_list.end(); it++) {
          if ((*it)->is_complete())
            completed++;
          else
            assigned++;
        }

        _queued   ->addcontent(1.,double(assigned+completed));
        _assigned ->addcontent(1.,double(assigned));
        _completed->addcontent(1.,double(completed));

        timespec now;
        clock_gettime(CLOCK_REALTIME,&now);
        ClockTime time(now.tv_sec,now.tv_nsec);

        _queued   ->time(time);
        _assigned ->time(time);
        _completed->time(time);
      }
    private:
      Pds::Appliance&           _app;
      std::vector<Work::Task*>  _tasks;
      Pds::Task&                _mgr_task;
      GenericPool               _pool;
      Semaphore                 _sem;
      bool                      _handled;
      std::list  <Work::Entry*> _list;
      MonEntryTH1F*             _start_to_complete;
      MonEntryTH1F*             _queued;
      MonEntryTH1F*             _assigned;
      MonEntryTH1F*             _completed;
    };
  };
};

using namespace Pds;

void Work::Task::routine() {
  bool lCaught=true;
  InDatagram* dg = (InDatagram*)_entry->ptr();
  try {
    process(dg);
    lCaught=false;
  }
  catch (std::exception& e) {
    printf("WorkThreads::Task::routine caught %s\n",e.what());
    _app.handle(e.what());
  }
  catch (std::string& e) {
    printf("WorkThreads::Task::routine caught %s\n",e.c_str());
    _app.handle(e.c_str());
  }
  catch (...) {
    printf("WorkThreads::Task::routine caught unknown exception\n");
    _app.handle("Unknown plugin exception");
  }

  if (lCaught)
    dg->datagram().xtc.damage.increase(Damage::UserDefined);

  _app.mgr_task().call(new Work::ComplEv(_entry,_app,_id));
}

void Work::QueueTr::routine() { _app.queueTransition(_tr); delete this; }
void Work::QueueEv::routine() { _app.queueEvent(_in); delete this; }
void Work::ComplEv::routine() { _app.completeEntry(_in,_id); delete this; }

WorkThreads::WorkThreads(const char* name,
                         const std::vector<Appliance*>& apps) :
  _mgr_task(new Task(TaskObject("Workmgr")))
{
  _mgr = new Work::Manager(name, *this, *_mgr_task);
  
  unsigned nthreads = apps.size();
  std::vector<Work::Task*> tasks(nthreads);
  for(unsigned id=0; id<nthreads; id++)
    tasks[id] = new Work::Task(id,*_mgr, apps[id]);

  _mgr->tasks(tasks);
}

WorkThreads::~WorkThreads()
{
  _mgr_task->destroy();
  delete _mgr;
}

Transition* WorkThreads::transitions(Transition* tr)
{
  _mgr_task->call(new Work::QueueTr(tr,*_mgr));
  return (Transition*)Appliance::DontDelete;
}

InDatagram* WorkThreads::events(InDatagram* in)
{
  _mgr_task->call(new Work::QueueEv(in,*_mgr));
  return (InDatagram*)Appliance::DontDelete;
}
