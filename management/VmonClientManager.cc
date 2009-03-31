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
#include "pdsdata/xtc/DetInfo.hh"

#include <errno.h>
#include <string.h>

using namespace Pds;


VmonClientManager::VmonClientManager(unsigned char platform,
				     const char* partition,
				     MonConsumerClient& consumer) :
  CollectionObserver(platform, partition, 0),
  _consumer (consumer),
  _socket   (new VmonClientSocket),
  _partition(-1),
  _state    (Disconnected)
{
  _socket->setrcvbuf(0x8000);
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


// MonFd interface
int VmonClientManager::fd() const { return _socket->socket(); }

int VmonClientManager::processIo()
{
  MonMessage reply(MonMessage::NoOp);
  _socket->peek(&reply);
  
  MonClient* client = lookup(reply.src());
  switch (reply.type()) {
  case MonMessage::Description: 
    client->read_description(reply.payload());
    client->use_all();
    break;
  case MonMessage::Payload:     
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

  char buff[128];
  if (src.level()==Level::Source) {
    const DetInfo& info = static_cast<const DetInfo&>(src);
    sprintf(buff,"%s.%d.%s.%d",
	    DetInfo::name(info.detector()),
	    info.detId(),
	    DetInfo::name(info.device()),
	    info.devId());
  }
  else {
    const ProcInfo& info = static_cast<const ProcInfo&>(src);
    sprintf(buff,"%s[%08x/%d]",
	    Level::name(info.level()),
	    info.ipAddr(),
	    info.processId());
  }

  MonClient* client = new MonClient(_consumer,
				    new MonCds(buff),
				    *_socket,
				    src);
  add(*client);
  _client_list.push_back(client);
  return client;
}

void VmonClientManager::clear()
{
  for(list<MonClient*>::iterator iter = _client_list.begin();
      iter != _client_list.end(); iter++)
    delete (*iter);
  _client_list.clear();
}
