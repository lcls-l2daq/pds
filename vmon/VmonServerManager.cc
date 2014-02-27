#include "pds/vmon/VmonServerManager.hh"

#include "pds/mon/MonMessage.hh"
#include "pds/mon/MonServer.hh"
#include "pds/vmon/VmonServerSocket.hh"
#include "pds/collection/Route.hh"

#include <errno.h>
#include <string.h>

using namespace Pds;


VmonServerManager::VmonServerManager(const char* name) :
  _cds   (name),
  _socket(0)
{
}


VmonServerManager::~VmonServerManager()
{
}

MonCds& VmonServerManager::cds()
{ return _cds; }

void VmonServerManager::listen(const Src& src, const Ins& ins)
{
  _socket = new VmonServerSocket(ins, Route::interface() );
  _server = new MonServer(src, cds(), _usage, *_socket);
  manage(*this);
}

void VmonServerManager::listen()
{
  if (_socket) {
    unmanage(*this);
    delete _server;
    delete _socket;
    _socket = 0;
  }
}

int VmonServerManager::fd() const { return _socket->socket(); }

int VmonServerManager::processIo()
{
  MonMessage request(MonMessage::NoOp);
  _socket->peek(&request);

  switch(request.type()) {
  case MonMessage::DescriptionReq:
    _server->description();
    break;
  case MonMessage::PayloadReq:
    _server->payload();
    break;
  default:
    break;
  }
  _socket->flush();
  return 1;
}

static VmonServerManager* _instance = 0;

VmonServerManager* VmonServerManager::instance(const char* name)
{
  if (!_instance)
    _instance = new VmonServerManager(name ? name : "Unknown");
  else if (name)
    const_cast<MonDesc&>(_instance->cds().desc()).name(name);
  return _instance;
}
