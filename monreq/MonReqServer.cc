#include "pds/monreq/MonReqServer.hh"
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>

#include "pds/service/Sockaddr.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/service/Ins.hh"
#include "pds/collection/Route.hh"
#include "pds/client/QueuedAction.hh"

using namespace Pds;


MonReqServer::MonReqServer(unsigned int nodenumber, unsigned int platform) :
  _task(new Task(TaskObject("monlisten"))), 
  _task2(new Task(TaskObject("que"))), 
  _servers(0),
  _connMgr(Route::interface(), StreamPorts::monRequest(platform), nodenumber, _servers),
  _sem(Semaphore::EMPTY),
  _counter(0),
  _sem2(Semaphore::FULL)
{
  _task->call(this);
  _id=nodenumber;
  _platform=platform;
}

MonReqServer::~MonReqServer()
{
  _task->destroy();
}

Transition* MonReqServer::transitions(Transition* tr)
{
  printf("MonReqServer transition %s\n",TransitionId::name(tr->id()));
  return tr;
}

void MonReqServer::routine()
{
  //
  //  Poll for new connection requests and new event requests
  //
  while(1) {
    int nfd=1+_servers.size();
    std::vector<pollfd> pfd(nfd);
    pfd[0].fd=_connMgr.socket();
    pfd[0].events = POLLIN|POLLERR;
    for(unsigned i=0; i<_servers.size(); i++) {
      pfd[i+1].fd = _servers[i].socket();
      pfd[i+1].events = POLLIN|POLLERR;
    }

    poll(&pfd[0], pfd.size(), 1000);
	
    if(pfd[0].revents&POLLIN) { //add new connections to array
      int fd=_connMgr.receiveConnection();
      if( fd<0) {
      }
      else {
        _servers.push_back(Pds::MonReq::ServerConnection(fd));
      }
    }

    for(unsigned j=1; j<pfd.size(); j++) {
      if(pfd[j].revents&POLLIN) {  //if there is new data from existing connection, recv it 
        if( _servers[j-1].recv() < 0){
          _servers.erase(_servers.begin()+j-1); 
          pfd.erase(pfd.begin()+j);
        }
      }
    }
	
  }

}  

InDatagram* MonReqServer::fire (InDatagram* dg) 
{
  //
  //  Pass the datagram to all connections
  //
  _sem2.take();
  _counter--;
  _sem2.give();
  for(unsigned i=0; i<_servers.size(); i++)
    _servers[i].send(dg->datagram());
  post(dg);
  return 0;
}

InDatagram* MonReqServer::events(InDatagram* in) 
{

  if (in->datagram().seq.service()==TransitionId::L1Accept) {
    //  Prevent the fire thread from holding all the buffers
    if (_counter < 32) {
      _task2->call(new QueuedAction(in,*this));
      _sem2.take();
      _counter++;
      _sem2.give();
    }
    else {
      return in;
    }
  }
  else {
    _task2->call(new QueuedAction(in,*this,&_sem));
    _sem.take();
    _sem2.take();
    _counter++;
    _sem2.give();
  }
  return (InDatagram*)Appliance::DontDelete;
}










