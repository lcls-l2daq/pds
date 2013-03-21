#include "pds/client/FrameCompApp.hh"

#include "pds/client/XtcStripper.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/service/Task.hh"

#include "pdsdata/compress/CompressedPayload.hh"
#include "pdsdata/compress/HistNEngine.hh"
#include "pdsdata/compress/Camera_FrameV1.hh"
#include "pdsdata/camera/FrameV1.hh"

#include <list>

namespace Pds {
  namespace FCA {
    class Entry {
    public:
      Entry(Transition* tr) : _state(Completed), _type(TypeT), _ptr(tr) {}
      Entry(InDatagram* in) : _state(in->datagram().seq.isEvent() ? Queued:Completed),
                              _type(TypeI), _ptr(in) {}
    public:
      bool is_complete  () const { return _state==Completed; }
      bool is_unassigned() const { return _state==Queued; }
    public:
      void assign  () { _state=Assigned; }
      void complete() { _state=Completed; }
      void post    (FrameCompApp& app) { 
        if (_type == TypeT) app.post((Transition*)_ptr);
        else                app.post((InDatagram*)_ptr);
      }
      void* ptr() { return _ptr; }
    private:
      enum { Queued, Assigned, Completed } _state;
      enum { TypeT, TypeI } _type;
      void* _ptr;
    };
    
    typedef std::list<Entry*> EList;

    class QueueTr : public Routine {
    public:
      QueueTr(Transition* tr, FrameCompApp& app) : _tr(tr), _app(app) {}
      void routine() { _app.queueTransition(_tr); delete this; }
    private:
      Transition*   _tr;
      FrameCompApp& _app;
    };

    class QueueEv : public Routine {
    public:
      QueueEv(InDatagram* in, FrameCompApp& app) : _in(in), _app(app) {}
      void routine() { _app.queueEvent(_in); delete this; }
    private:
      InDatagram*   _in;
      FrameCompApp& _app;
    };

    class ComplEv : public Routine {
    public:
      ComplEv(FCA::Entry* in,FrameCompApp& app,unsigned id) : _in(in), _app(app), _id(id) {}
      void routine() { _app.completeEntry(_in,_id); delete this; }
    private:
      FCA::Entry*   _in;
      FrameCompApp& _app;
      unsigned      _id;
    };

    class MyIter : public XtcStripper {
    public:
      enum Status {Stop, Continue};
      MyIter(Xtc* xtc, uint32_t*& p, char* obuff, size_t max_osize) :
        XtcStripper(xtc, p), _obuff(obuff), _max_osize(max_osize) {}
      ~MyIter() {}
    protected:
      void process(Xtc*);
    private:
      char*  _obuff;
      size_t _max_osize;
    };

    class Task : public Routine {
    public:
      Task(unsigned id, FrameCompApp& app, size_t max_size) :
        _id(id), _app(app), 
        _task(new Pds::Task(TaskObject("FCAtsk"))),
        _entry(0),
        _obuff(new uint32_t[max_size>>2]),
        _max_size(max_size) {}
      ~Task() { _task->destroy(); delete[] _obuff; }
    public:
      void assign(FCA::Entry* e) { (_entry = e)->assign(); _task->call(this); }
      void unassign() { _entry = 0; }
      bool unassigned() const { return _entry==0; }
      void routine() {
        InDatagram* in = (InDatagram*)_entry->ptr();
        uint32_t* pdg = reinterpret_cast<uint32_t*>(&(in->datagram().xtc));
        MyIter iter(&in->datagram().xtc,pdg,(char*)_obuff,_max_size);
        iter.iterate();
        _app.mgr_task().call(new ComplEv(_entry,_app,_id));
      }
      unsigned id() const { return _id; }
    private:
      unsigned      _id;
      FrameCompApp& _app;
      Pds::Task*    _task;
      FCA::Entry*   _entry;
      enum { MaxSize = 0x2000000 };
      uint32_t*     _obuff;
      size_t        _max_size;
    };

  };
};

using namespace Pds;

FrameCompApp::FrameCompApp(size_t max_size) :
  _mgr_task(new Task(TaskObject("FCAmgr"))),
  _tasks   (4)
{
  for(unsigned id=0; id<_tasks.size(); id++)
    _tasks[id] = new FCA::Task(id,*this,max_size);
}

FrameCompApp::~FrameCompApp()
{
  _mgr_task->destroy();
  for(unsigned id=0; id<_tasks.size(); id++)
    delete _tasks[id];
}

Transition* FrameCompApp::transitions(Transition* tr)
{
  _mgr_task->call(new FCA::QueueTr(tr,*this));
  return (Transition*)Appliance::DontDelete;
}

InDatagram* FrameCompApp::events(InDatagram* in)
{
  _mgr_task->call(new FCA::QueueEv(in,*this));
  return (InDatagram*)Appliance::DontDelete;
}

void FrameCompApp::queueTransition(Transition* tr)
{
  _list.push_back(new FCA::Entry(tr));
  _process();
}

void FrameCompApp::queueEvent(InDatagram* in)
{
  _list.push_back(new FCA::Entry(in));
  _process();
}

void FrameCompApp::completeEntry(FCA::Entry* e, unsigned id)
{
  e->complete();
  _tasks[id]->unassign();
  _process();
}

void FrameCompApp::_process()
{
  //  First post the entries in order that are complete
  while( !_list.empty() ) {
    FCA::Entry* e=_list.front();
    if (!e->is_complete()) break;

    _list.pop_front();
    e->post(*this);
  }

  //  Next assign entries in order that haven't yet been
  for(std::list<FCA::Entry*>::iterator it=_list.begin(); it!=_list.end(); it++) {
    if ((*it)->is_unassigned()) {
      bool lassign=false;
      for(unsigned id=0; id<_tasks.size(); id++)
        if (_tasks[id]->unassigned()) {
          _tasks[id]->assign(*it);
          lassign=true;
          break;
        }
      if (!lassign) break;
    }
  }
}



void FCA::MyIter::process(Xtc* xtc) 
{
  if (xtc->contains.id()==TypeId::Id_Xtc) {
    FCA::MyIter iter(xtc,_pwrite,_obuff,_max_osize);
    iter.iterate();
    return;
  }

  if (xtc->contains.id()==TypeId::Id_Frame) {
    switch(xtc->contains.version()) {
    case 1: {
      const Camera::FrameV1& frame = *reinterpret_cast<const Camera::FrameV1*>(xtc->payload());

      Compress::HistNEngine engine;
      size_t outsz;
      if (engine.compress(frame.data(), frame.depth_bytes(), frame.data_size(),
                          _obuff,
                          outsz) != Compress::HistNEngine::Success) {
        break;
      }
      //  Require data size is reduced
      else if (outsz + sizeof(CompressedPayload) > frame.data_size()) {
        break;
      }


      Xtc nxtc( TypeId(xtc->contains.id(), xtc->contains.version(), true),
                xtc->src,
                xtc->damage );

      Camera::CompressedFrameV1 cframe(frame);

      const unsigned align_mask = sizeof(uint32_t)-1;
      unsigned spayload = (outsz+align_mask)&~align_mask;
      nxtc.extent = sizeof(Xtc) + sizeof(cframe) + spayload;

      _write(&nxtc, sizeof(Xtc));
      _write(&cframe, sizeof(cframe)-sizeof(CompressedPayload));

      CompressedPayload pd(CompressedPayload::HistN,frame.data_size(),outsz);
      _write(&pd, sizeof(pd));

      _write(_obuff, spayload);

      return; }
    default:
      break;
    }
  }

  XtcStripper::process(xtc);
}



