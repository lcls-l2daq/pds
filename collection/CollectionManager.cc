#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "CollectionManager.hh"
#include "CollectionServer.hh"
#include "CollectionPorts.hh"
#include "Message.hh"
#include "Route.hh"
#include "Arp.hh"

#include "pds/service/Task.hh"
#include "pds/service/TaskWait.hh"

using namespace Pds;

static void print_error(const char* where, int error) 
{
  printf("*** %s error: %s\n", where, strerror(error));
}

static const int TaskPriority = 55;
static const char* TaskName[Level::NumberOfLevels+1] = {
  "oCllCtr", "oCllSrc", "oCllSeg", "oCllFrg", "oCllEvt", "oCllObs"
};

static const int Disconnect = 1;
static const int Cancel = 2;

static const unsigned TempCrates = 0x1;

CollectionManager::CollectionManager(Level::Type level,
                                     unsigned partition,
                                     unsigned maxpayload,
                                     unsigned timeout,
                                     Arp* arp) :
  ServerManager(Ins(), sizeof(Node), maxpayload, timeout),
  _task(new Task(TaskObject(TaskName[level], TaskPriority))),
  _driver(*this),
  _header(level, partition), 
  _client(sizeof(Node), maxpayload),
  _payload(new char[maxpayload]),
  _mcastServer(0),
  _ucastServer(0),
  _arp(arp)
{}

CollectionManager::CollectionManager(unsigned maxpayload,
                                     Arp* arp) :
  ServerManager(PdsCollectionPorts::platform(), sizeof(Node), maxpayload, 0),
  _task(new Task(TaskObject(TaskName[Level::Source], TaskPriority))),
  _driver(*this),
  _header(Level::Source, 0), 
  _client(sizeof(Node), maxpayload),
  _payload(new char[maxpayload]),
  _mcastServer(0),
  _ucastServer(0),
  _arp(arp)
{
  Route::set(_table, _table.interface(0));
  NetServer::join(PdsCollectionPorts::platform(), Route::interface());
  _build_servers();
  donottimeout();
}

CollectionManager::~CollectionManager() 
{
  if (_header.level() == Level::Source) {
    NetServer::resign();
  }
  _destroy_servers();
  delete [] _payload;
  _task->destroy();
}

void CollectionManager::connect() 
{
  _driver.run(*_task);
  if (timeout()) {
    Message msg(Message::Join);
    Node header(_header);
    if (_table.routes() > 1) {
      for (unsigned index=0; index<_table.routes(); index++) {
        header.fixup(_table.interface(index), _table.ether(index));
        msg.reply_to(Ins(_table.interface(index), portId()));
        Ins dst = PdsCollectionPorts::platform();
        _client.use(_table.interface(index));
        _send(msg, dst, header);
      }
    } else {
      Route::set(_table, _table.interface(0));
      _client.use(Route::interface());
      _build_servers();
      connected(header, msg);
    }
  }
}

void CollectionManager::disconnect() 
{
  unblock((char*)&Disconnect);
}

void CollectionManager::cancel() 
{
  TaskWait block(_task);
  unblock((char*)&Cancel);
  block.wait();
}

void CollectionManager::mcast(const Message& msg)
{
  Message& tmp = const_cast<Message&>(msg);
  tmp.reply_to(Ins(_header.ip(), _ucastServer->portId()));
  _send(msg, _mcast);
}

void CollectionManager::ucast(const Message& msg, const Ins& dst)
{
  Message& tmp = const_cast<Message&>(msg);
  tmp.reply_to(Ins(_header.ip(), _ucastServer->portId()));
  _send(msg, dst);
}

const Node& CollectionManager::header() const {return _header;}

char* CollectionManager::payload() {return _payload;}

int CollectionManager::commit(char* datagram,
                              char* payload,
                              int   payloadSize,
                              const Ins& src) 
{
  if (!payloadSize) {
    int request = *(int*)datagram;
    switch (request) {
    case Disconnect:
      disconnected();
      break;
    default:
      break;
    }
    return 0;
  }
  const Node& hdr = *(Node*)datagram;
  const Message& msg = *(Message*)payload;
  if (_header.level() != Level::Source) {
    Route::set(_table, hdr.ip());
    _client.use(Route::interface());
    _build_servers();
    connected(hdr, msg);
  } else {
    arpadd(hdr);
    Message rep(Message::Join);
    ucast(rep, msg.reply_to());
  }
  return 1;
}

int CollectionManager::handleError(int value)
{
  print_error("CollectionManager handleError", value);
  return 1;
}

int CollectionManager::processIo(Server* srv) 
{
  printf("CollectionManager::processIo srvId %d\n",srv->id());
  return srv->pend(MSG_DONTWAIT);
}

int CollectionManager::processTmo() 
{
  timedout();
  arm(); // Otherwise only OOB server would be re-enabled
  return 1;
}

void CollectionManager::_send(const Message& msg, 
                              const Ins& dst) 
{
  int error = _client.send((char*)&_header, (char*)&msg, msg.size(), dst);
  if (error) print_error("CollectionManager send", error);
}

void CollectionManager::_send(const Message& msg, 
                              const Ins& dst,
                              const Node& hdr) 
{
  int error = _client.send((char*)&hdr, (char*)&msg, msg.size(), dst);
  if (error) print_error("CollectionManager send", error);
}

void CollectionManager::_build_servers() 
{
  _header.fixup(Route::interface(), Route::ether());
  _mcast = PdsCollectionPorts::collection(_header.partition());
  _destroy_servers();
  const unsigned Datagrams = 256;
  const unsigned BroadcastServerId = 0;
  _mcastServer =
    new CollectionServer(*this, BroadcastServerId, _mcast, 
			    _client.maxPayload(), Datagrams);
  _mcastServer->join(_mcast, Route::interface());
  if (_mcastServer->error()) {
    print_error("CollectionManager bc create", _mcastServer->error());
  } else {
    manage(_mcastServer);
    arm(_mcastServer);
  }
  const unsigned UnicastServerId = 1;
  _ucastServer =
    new CollectionServer(*this, UnicastServerId, Route::interface(), 
			    _client.maxPayload(), Datagrams);
  if (_ucastServer->error()) {
    print_error("CollectionManager uc create", _ucastServer->error());
  } else {
    manage(_ucastServer);
    arm(_ucastServer);
  }
}

void CollectionManager::_destroy_servers() 
{
  if (_ucastServer) {
    unmanage(_mcastServer);
    unmanage(_ucastServer);
    delete _mcastServer;
    delete _ucastServer;
    _mcastServer = 0;
    _ucastServer = 0;
  }
}

void CollectionManager::arpadd(const Node& hdr) 
{  
  if (_arp && hdr.ip() != _header.ip()) _arp->add(hdr);
}
