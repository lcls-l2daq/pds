#include "pds/management/EventAppCallback.hh"
#include "pds/utility/Appliance.hh"
#include "pds/utility/Stream.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pds/collection/Node.hh"
#include "pds/service/Task.hh"

using namespace Pds;

EventAppCallback::EventAppCallback(Task*            task,
                                   unsigned         platform,
                                   Appliance&       app) :
  _task    (task),
  _platform(platform)
{
  _app.push_back(&app);
}

EventAppCallback::EventAppCallback(Task*                 task,
                                   unsigned              platform,
                                   std::list<Appliance*> app) :
  _task    (task),
  _platform(platform),
  _app     (app)
{
}

EventAppCallback::~EventAppCallback()
{
  _task->destroy();
}

void EventAppCallback::attached(SetOfStreams& streams)
{
  printf("Seg connected to platform 0x%x\n",_platform);
  
  Stream* frmk = streams.stream(StreamParams::FrameWork);
  for(std::list<Appliance*>::iterator it=_app.begin(); 
      it!=_app.end(); it++)
    (*it)->connect(frmk->inlet());
}

void EventAppCallback::failed(Reason reason)
{
  static const char* reasonname[] = { "platform unavailable", 
                                      "crates unavailable", 
                                      "fcpm unavailable" };
  printf("Seg: unable to allocate crates on platform 0x%x : %s\n", 
         _platform, reasonname[reason]);
  delete this;
}

void EventAppCallback::dissolved(const Node& who)
{
  const unsigned userlen = 12;
  char username[userlen];
  Node::user_name(who.uid(),username,userlen);
  
  const unsigned iplen = 64;
  char ipname[iplen];
  Node::ip_name(who.ip(),ipname, iplen);
  
  printf("Seg: platform 0x%x dissolved by user %s, pid %d, on node %s", 
         who.platform(), username, who.pid(), ipname);
  
  delete this;
}
    
