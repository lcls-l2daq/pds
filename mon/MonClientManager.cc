#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>

#include "pds/mon/MonClientManager.hh"
#include "pds/mon/MonStreamClient.hh"
#include "pds/mon/MonFd.hh"
#include "pds/mon/MonPort.hh"
#include "pds/mon/MonSrc.hh"
#include "pds/mon/MonStreamSocket.hh"
#include "pds/mon/MonConsumerClient.hh"
#include "pds/service/Task.hh"


static const int DonotTimeout = -1;

using namespace Pds;

static Ins lookup_ins(const char* hostname, MonPort::Type type)
{
  unsigned short port = MonPort::port(type);
  int address = 0;
  hostent* host = gethostbyname(hostname);
  if (host && host->h_addr_list[0]) {
    address = ntohl(*(int*)host->h_addr_list[0]);
  }
  return Ins(address, port);
}

class MonClientManagerMsg {
public:
  enum Msg {Noop, Shutdown, Connect, Disconnect};
  MonClientManagerMsg(Msg msg=Noop, MonClient* client=0) :
    _msg(msg),
    _client(client)
  {}
  
  Msg msg() {return _msg;}
  MonClient& client() {return *_client;}

private:
  Msg _msg;
  MonClient* _client;
};

MonClientManager::MonClientManager(MonConsumerClient& consumer,
				   const char** hosts) :
  MonPoll(DonotTimeout),
  _consumer(consumer),
  _nclients(0),
  _task(new Task(TaskObject("oClntMgr"))),
  _sem (Semaphore::EMPTY)
{
  if (socket() < 0) {
    printf("*** odfPoll::ctor error with loopback socket: %s\n", 
	   strerror(errno));
  } else {
    _task->call(this);
  }
  for (unsigned p=0; p<MonPort::NTypes; p++) {
    if (hosts[p]) _nclients++;
  }
  _cds     = new MonCds*[_nclients];
  _clients = new MonClient*[_nclients];
  for (unsigned p=0, s=0; p<MonPort::NTypes; p++) {
    if (hosts[p]) {
      MonPort::Type type = MonPort::Type(p);
      _clients[s] = new MonStreamClient(consumer, 
					new MonCds(MonPort::name(type)),
					new MonStreamSocket,
					MonSrc(s));
      _clients[s]->dst(lookup_ins(hosts[p],type));
      s++;
    }
  }
}

MonClientManager::~MonClientManager() 
{
  if (socket() >= 0) {
    MonClientManagerMsg msg(MonClientManagerMsg::Shutdown);
    write(&msg, sizeof(msg));
    _sem.take();
  }
  for (unsigned s=0; s<_nclients; s++) {
    delete _clients[s];
  }
  delete [] _cds;
  delete [] _clients;
  _task->destroy();
}

void MonClientManager::connect(MonClient& client)
{ 
  MonClientManagerMsg msg(MonClientManagerMsg::Connect, &client);
  write(&msg, sizeof(msg));
}

void MonClientManager::disconnect(MonClient& client)
{ 
  MonClientManagerMsg msg(MonClientManagerMsg::Disconnect, &client);
  write(&msg, sizeof(msg));
}


int MonClientManager::processMsg()
{
  MonClientManagerMsg msg;
  if (read(&msg, sizeof(msg)) > 0) {
    switch (msg.msg()) {
    case MonClientManagerMsg::Shutdown:
      _sem.give();
      return 0;
    case MonClientManagerMsg::Connect:
      {
	MonClient& client = msg.client();
	MonStreamSocket& socket = static_cast<MonStreamSocket&>(client.socket());
	if (socket.connect(client.dst()) < 0) {
	  _consumer.process(client, MonConsumerClient::ConnectError, 
			    errno);
	} else {
	  socket.setrcvbuf(0x8000);
	  manage(client);
	  _consumer.process(client, MonConsumerClient::Connected);
	}
      }
      break;
    case MonClientManagerMsg::Disconnect:
      {
	MonClient& client = msg.client();
	unmanage(client);
	if (client.socket().close() < 0) {
	  _consumer.process(client, MonConsumerClient::DisconnectError, 
			    errno);
	} else {
	  _consumer.process(client, MonConsumerClient::Disconnected);
	}
      }
      break;
    default:
      break;
    }
  }
  return 1;
}

int MonClientManager::processTmo() 
{
  return 1;
}

int MonClientManager::processFd() 
{
  shrink();
  return 1;
}

int MonClientManager::processFd(MonFd& fd) 
{
  MonClient& client = (MonClient&)fd;
  unmanage(client);
  client.socket().close();
  return 0;
}

void MonClientManager::routine()
{
  while (poll());
}

unsigned MonClientManager::nclients() const {return _nclients;}

MonClient* MonClientManager::client(unsigned c)
{
  if (c < _nclients)
    return _clients[c];
  else
    return 0;
}

const MonClient* MonClientManager::client(unsigned c) const
{
  if (c < _nclients)
    return _clients[c];
  else
    return 0;
}

MonClient* MonClientManager::client(const char* name)
{
  for (unsigned s=0; s<_nclients; s++) {
    if (strcmp(_clients[s]->cds().desc().name(), name) == 0) 
      return _clients[s];
  }
  return 0;
}
