#ifndef Pds_FrameCompApp_hh
#define Pds_FrameCompApp_hh

/*
**  This appliance will compress images in a pipeline that distributes the
**  work among a fixed number of threads.  Each thread gets the whole job
**  of compressing one event.  The events are completed in order.
*/

#include "pds/utility/Appliance.hh"

#include <list>
#include <vector>

namespace Pds {
  class MonEntryTH1F;
  class Task;

  namespace FCA { class Entry; class Task; class Timer; }

  class FrameCompApp : public Appliance {
  public:
    FrameCompApp(size_t max_size, unsigned nthreads=4);
    ~FrameCompApp();
  public:
    Transition* transitions(Transition*);
    InDatagram* events     (InDatagram*);
  public:
    Task& mgr_task() { return *_mgr_task; }
    void  queueTransition(Transition*);
    void  queueEvent     (InDatagram*);
    void  completeEntry  (FCA::Entry*,unsigned);
    void  process        ();
    void  audit          ();
  public:
    static void useOMP(bool);
    static void setVerbose(bool);
    static void setCopyPresample(unsigned);
  private:
    void  _post(FCA::Entry*);
  private:
    Pds::Task*              _mgr_task;
    std::list<FCA::Entry*>  _list;
    std::vector<FCA::Task*> _tasks;
    FCA::Timer*             _timer;
    MonEntryTH1F* _start_to_complete;
    MonEntryTH1F* _start_to_post;
    MonEntryTH1F* _compress_ratio;
    MonEntryTH1F* _queued;
    MonEntryTH1F* _assigned;
    MonEntryTH1F* _completed;
  };
};

#endif
