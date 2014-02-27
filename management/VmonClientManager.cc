#include "pds/management/VmonClientManager.hh"

#include "pds/service/Sockaddr.hh"
#include "pds/service/Ins.hh"

#include "pds/collection/Route.hh"

#include "pds/utility/StreamPorts.hh"
#include "pds/utility/Transition.hh"

#include "pds/mon/MonCds.hh"
#include "pds/mon/MonClient.hh"
#include "pds/mon/MonMessage.hh"
#include "pds/mon/MonConsumerClient.hh"
#include "pds/vmon/VmonClientSocket.hh"
#include "pds/xtc/InDatagram.hh"

#include <errno.h>
#include <string.h>

using namespace Pds;


VmonClientManager::VmonClientManager(unsigned char platform,
				     const char* partition,
				     MonConsumerClient& consumer) :
  CollectionObserver(platform, partition),
  _consumer (consumer),
  _socket   (new VmonClientSocket),
  _partition(-1),
  _state    (Disconnected)
{
  _socket->setrcvbuf(0x80000);
}


VmonClientManager::~VmonClientManager()
{
}


void VmonClientManager::connect(int partition) 
{
  disconnect();

  _partition = partition;
  Ins vmon(StreamPorts::vmon(_partition));

  _socket->set_dst(vmon, Route::interface());

  _poll.manage(*this);
  //      _consumer.process(*_client, MonConsumerClient::Connected);
  _state = Connected;
}

void VmonClientManager::map()
{
  if (_state == Connected) {
    //    _consumer.process(*_client, MonConsumerClient::Connected);
    _state = Mapped;

    MonMessage request(MonMessage::DescriptionReq);
    _socket->write(&request, sizeof(request));
  }
}


void VmonClientManager::disconnect()
{
  if (_state != Disconnected) {
    _poll.unmanage(*this);
    clear();
    _state = Disconnected;
  }
}


//  CollectionObserver interface
void VmonClientManager::post(const Transition& tr)
{
  if (tr.id()==TransitionId::Map) {
    const Allocate& allocate = static_cast<const Allocate&>(tr);
    connect(allocate.allocation().partitionid());
  }
  else if (tr.id()==TransitionId::Configure) {
    map();
  }
  else if (tr.id()==TransitionId::Unmap) {
    disconnect();
  }
  delete const_cast<Transition*>(&tr);
}

void VmonClientManager::post(const InDatagram& in)
{
  delete const_cast<InDatagram*>(&in);
}


// MonFd interface
int VmonClientManager::fd() const { return _socket->socket(); }

int VmonClientManager::processIo()
{
  MonMessage reply(MonMessage::NoOp);
  _socket->peek(&reply);
  
  MonClient* client = lookup(reply.src());
  switch (reply.type()) {
  case MonMessage::Description: 
    { if (!client) {
        const Src& src = reply.src();
        client = new MonClient(_consumer,
                               reply.payload(),
                               *_socket,
                               src);
        add(*client);
        _client_list.push_back(client);
      }
      else 
        client->read_description(reply.payload());
    }
    client->use_all();
    break;
  case MonMessage::Payload:     
    if (!client) {
      printf("Unknown client contributes payload\n");
      abort();
    }
    client->read_payload(); 
    break;
  default:          
    break;
  }
  _socket->flush(); 
  return 1;
}

void VmonClientManager::request_payload()
{
  if (_state==Mapped) {
    MonMessage request(MonMessage::PayloadReq);
    _socket->write(&request, sizeof(request));
  }
}

MonClient* VmonClientManager::lookup(const Src& src)
{
  for(list<MonClient*>::iterator iter = _client_list.begin();
      iter != _client_list.end(); iter++) {
    if ((*iter)->src()==src) return (*iter);
  }
  return 0;
}

void VmonClientManager::clear()
{
  for(list<MonClient*>::iterator iter = _client_list.begin();
      iter != _client_list.end(); iter++)
    delete (*iter);
  _client_list.clear();
}
