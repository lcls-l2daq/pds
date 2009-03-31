#include "pds/mon/MonStreamClient.hh"
#include "pds/mon/MonStreamSocket.hh"

using namespace Pds;

MonStreamClient::MonStreamClient(MonConsumerClient& consumer, 
				 MonCds* cds,
				 MonStreamSocket* socket,
				 const Src& src) :
  MonClient(consumer, cds, *socket, src),
  _ssocket(socket)
{
}

MonStreamClient::~MonStreamClient()
{
  delete _ssocket;
}
