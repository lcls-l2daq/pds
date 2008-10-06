#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "CollectionServer.hh"
#include "CollectionServerManager.hh"
#include "Node.hh"
#include "Message.hh"
#include "Route.hh"

using namespace Pds;

CollectionServer::CollectionServer(CollectionServerManager& manager,
				   int id,
				   int interface,
				   unsigned maxpayload,
				   unsigned datagrams) :
  NetServer(id, Ins(interface), sizeof(Node), 
	    maxpayload, datagrams), 
  _manager(manager),
  _payload(new char[maxpayload])
{}

CollectionServer::CollectionServer(CollectionServerManager& manager,
				   int id,
				   const Ins& bcast,
				   unsigned maxpayload,
				   unsigned datagrams) :
  NetServer(id, Port::VectoredServerPort, bcast, sizeof(Node),
	    maxpayload, datagrams), 
  _manager(manager),
  _payload(new char[maxpayload])
{}

CollectionServer::~CollectionServer()
{
  delete [] _payload;
}

int CollectionServer::commit(char* datagram, 
                             char* payload, 
                             int size, 
                             const Ins& src) 
{
  const Node& hdr = *(Node*)datagram;
  const Message& msg = *(Message*)payload;
//   printf("<%s> (srv) from level %d platform %d ip 0x%x pid %d\n",
// 	 msg.type_name(), hdr.level(), hdr.platform(), hdr.ip(), hdr.pid());
  _manager.message(hdr, msg);
  return 1;
}

int CollectionServer::handleError(int error)
{
  if (error != EAGAIN) 
    printf("*** CollectionServer error: %s\n", strerror(error));
  return 1; 
}
