#include "CollectionServerManager.hh"
#include "CollectionServer.hh"
#include "Message.hh"
#include "Route.hh"
#include "Arp.hh"

#include "pds/service/Task.hh"
#include "pds/service/TaskWait.hh"

#include <string.h>
#include <stdio.h>
#include <errno.h>

using namespace Pds;

static void print_error(const char* where, int error) 
{
  printf("*** %s error: %s\n", where, strerror(error));
}

static const int TaskPriority = 55;
static const char* TaskName[Level::NumberOfLevels] = {
  "oCllCtr", "oCllSrc", "oCllSeg", "oCllEvt", "oCllRec", "oCllObs", "oClRep"
};

static const int Cancel = 1;

CollectionServerManager::CollectionServerManager(const Ins& mcast,
                                                 Level::Type level,
                                                 unsigned platform,
                                                 unsigned maxpayload,
                                                 unsigned timeout,
                                                 Arp* arp) :
  ServerManager(Ins(), sizeof(Node), maxpayload, timeout),
  _task(new Task(TaskObject(TaskName[level], TaskPriority))),
  _driver(*this),
  _header(level, platform), 
  _client(sizeof(Node), maxpayload),
  _payload(new char[maxpayload]),
  _arp(arp),
  _mcast(mcast),
  _mcastServer(0),
  _ucastServer(0)
{}

CollectionServerManager::~CollectionServerManager() 
{
  delete _mcastServer;
  delete _ucastServer;
  delete [] _payload;
  _task->destroy();
}

void CollectionServerManager::start() 
{
  _driver.run(*_task);
}

void CollectionServerManager::cancel() 
{
  TaskWait block(_task);
  unblock((char*)&Cancel);
  block.wait();
}

void CollectionServerManager::ucast(Message& msg, const Ins& dst)
{
  msg.reply_to(Ins(_header.ip(), _ucastServer->portId()));
  _send(msg, dst);
}

void CollectionServerManager::mcast(Message& msg)
{
  msg.reply_to(Ins(_header.ip(), _ucastServer->portId()));
  _send(msg, _mcast);
}

void CollectionServerManager::message(const Node& hdr, const Message& msg) {}

const Node& CollectionServerManager::header() const {return _header;}

char* CollectionServerManager::payload() {return _payload;}

int CollectionServerManager::handleError(int value)
{
  print_error("CollectionServerManager handleError", value);
  return 1;
}

int CollectionServerManager::processIo(Server* srv) 
{
  return srv->pend(MSG_DONTWAIT);
}

void CollectionServerManager::_send(const Message& msg, 
                                    const Ins& dst) 
{
  int error = _client.send((char*)&_header, (char*)&msg, msg.size(), dst);
  if (error) print_error("CollectionServerManager send", error);
}

void CollectionServerManager::_send(const Message& msg, 
                                    const Ins& dst,
                                    const Node& hdr) 
{
  int error = _client.send((char*)&hdr, (char*)&msg, msg.size(), dst);
  if (error) print_error("CollectionServerManager send", error);
}

void CollectionServerManager::arpadd(const Node& hdr) 
{  
  if (_arp && hdr.ip() != _header.ip()) _arp->add(hdr);
}

void CollectionServerManager::_connected() 
{
  int interface = Route::interface();
  _client.use(interface);
  _header.fixup(interface, Route::ether());
  if (!_ucastServer) {
    const unsigned Datagrams = 4;
    const unsigned BroadcastServerId = 0;
    _mcastServer =
      new CollectionServer(*this, BroadcastServerId, _mcast, 
                           _client.maxPayload(), Datagrams);
    _mcastServer->join(_mcast, interface);
    if (_mcastServer->error()) {
      print_error("CollectionManager bc create", _mcastServer->error());
    } else {
      manage(_mcastServer);
      arm(_mcastServer);
    }
    const unsigned UnicastServerId = 1;
    _ucastServer =
      new CollectionServer(*this, UnicastServerId, interface, 
                           _client.maxPayload(), Datagrams);
    if (_ucastServer->error()) {
      print_error("CollectionManager uc create", _ucastServer->error());
    } else {
      manage(_ucastServer);
      arm(_ucastServer);
    }
  }
}

void CollectionServerManager::_disconnected() 
{
  if (_ucastServer) {
    _mcastServer->resign();
    unmanage(_mcastServer);
    unmanage(_ucastServer);
    delete _mcastServer;
    delete _ucastServer;
    _mcastServer = 0;
    _ucastServer = 0;
  }
}
