#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "pds/mon/MonServerManager.hh"
#include "pds/mon/MonServer.hh"
#include "pds/mon/MonEntry.hh"

#include "pds/service/Task.hh"
#include "pds/service/Ins.hh"

static const int DonotTimeout = -1;
static const unsigned short Step = 64;

static const int Shutdown   = 0;
static const int Serve      = 1;
static const int Dontserve  = 2;
static const int Update     = 3;
static const int Enable     = 4;
static const int Disable    = 5;

using namespace Pds;

MonServerManager::MonServerManager(MonPort::Type type) :
  MonPoll(DonotTimeout),
  _cds(type),
  _listener(),
  _maxservers(Step),
  _nservers(0),
  _servers(new MonServer*[_maxservers]),
  _task(new Task(TaskObject("oSrvMgrTcp"))),
  _sem(Semaphore::EMPTY)
{
  if (socket() < 0) {
    printf("MonServerManager::ctor socket error in odfPoll ctor: %s\n", 
	   strerror(errno));
  } else {
    _task->call(this);
  }
}

MonServerManager::~MonServerManager() 
{
  if (socket() >= 0) {
    write(&Shutdown, sizeof(int));
    _sem.take();
  }
  delete [] _servers;
  _task->destroy();
}

int MonServerManager::serve()
{
  write(&Serve, sizeof(int));
  _sem.take();
  return _result;
}

int MonServerManager::dontserve()
{
  write(&Dontserve, sizeof(int));
  _sem.take();
  return _result;
}

void MonServerManager::enable()
{
  write(&Enable, sizeof(int));
  _sem.take();
}

void MonServerManager::disable()
{
  write(&Disable, sizeof(int));
  _sem.take();
}

void MonServerManager::update()
{
  write(&Update, sizeof(int));
}

MonCds& MonServerManager::cds() {return _cds;}
const MonCds& MonServerManager::cds() const {return _cds;}

int MonServerManager::fd() const { return _listener.socket(); }

int MonServerManager::processIo()
{
  int socket = _listener.accept();
  if (socket >= 0) {
    add(socket);
  } else {
    printf("*** MonServerManager::processIo accept error: %s\n", 
	   strerror(errno));
  }
  return 1;
}

int MonServerManager::processMsg() 
{
  int msg;
  if (read(&msg, sizeof(int)) > 0) {
    switch (msg) {
    case Shutdown:
      _sem.give();
      return 0;
    case Serve:
      {
	unsigned short port = MonPort::port(_cds.type());
	Ins src(port);
	if (_listener.listen(src) < 0) {
	  _result = errno;
	} else {
	  manage(*this); 
	  _result = 0;
	}
      }
      _sem.give();
      break;
    case Dontserve:
      {
	unmanage(*this); 
	if (_listener.close() < 0) {
	  _result = errno;
	} else {
	  _result = 0;
	}
	for (unsigned short s=0; s<_nservers; s++) {
	  MonServer* server = _servers[s];
	  unmanage(*server);
	  delete server;
	}
	_nservers = 0;
      }
      _sem.give();
      break;
    case Update:
      update_entries();
      break;
    case Enable:
      enable_servers();
      _sem.give();
      break;
    case Disable:
      disable_servers();
      _sem.give();
      break;
    }
  }
  return 1;
}

int MonServerManager::processTmo() 
{
  return 1;
}

int MonServerManager::processFd() 
{
  shrink();
  return 1;
}

int MonServerManager::processFd(MonFd& fd)
{
  for (unsigned short s=0; s<_nservers; s++) {
    if (_servers[s] == &fd) {
      remove(s);
      return 0;
    }
  }
  return 1;
}

void MonServerManager::routine()
{
  while (poll());
}

void MonServerManager::update_entries()
{
  bool disable = false;
  for (unsigned short u=0; u<_usage.used(); u++) {
    MonEntry* entry = _cds.entry(_usage.signature(u)); 
    if (entry->update() > 0) disable = true;
  }
  _usage.reset();
  if (disable) {
    disable_servers();
    enable_servers();
  }
}

void MonServerManager::disable_servers() 
{
  for (unsigned short s=0; s<_nservers; s++) _servers[s]->disable();
}

void MonServerManager::enable_servers() 
{
  for (unsigned short s=0; s<_nservers; s++) _servers[s]->enable();
}

void MonServerManager::adjust()
{
  unsigned short maxservers = _maxservers + Step;
  MonServer** servers = new MonServer*[maxservers];
  memcpy(servers, _servers, _nservers*sizeof(MonServer*));
  delete [] _servers;
  _servers = servers;
  _maxservers = maxservers;
}


void MonServerManager::add(int socket) 
{
  if (_nservers == _maxservers) adjust();
  MonServer* server = new MonServer(_cds, _usage, socket);
  server->setsndbuf(0x8000);
  manage(*server);
  _servers[_nservers] = server;
  _nservers++;
  printf("MonServerManager server %d created with socket [%d] (total %d)\n", 
	 _nservers-1, socket, _nservers);
}

void MonServerManager::remove(unsigned short pos) 
{
  MonServer* server = _servers[pos];
  int socket = server->socket();
  unmanage(*server);
  delete server;
  unsigned short p = pos+1;
  if (p < _nservers) {
    memmove(_servers+pos, _servers+p, (_nservers-p)*sizeof(MonServer*));
  }
  _nservers--;
  printf("MonServerManager server %d with socket [%d] closed (total %d)\n", 
	 pos, socket, _nservers);
}
