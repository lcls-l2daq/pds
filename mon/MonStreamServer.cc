#include "pds/mon/MonStreamServer.hh"
#include "pds/mon/MonStreamSocket.hh"

using namespace Pds;

MonStreamServer::MonStreamServer(const Src& src,
				 const MonCds& cds, 
				 MonUsage& usage, 
				 MonStreamSocket* socket) :
  MonServer(src, cds, usage, *socket),
  _socket(socket),
  _request(src,MonMessage::NoOp)
{
}

MonStreamServer::~MonStreamServer()
{
  delete _socket;
}

int MonStreamServer::fd() const { return _socket->socket(); }

int MonStreamServer::processIo()
{
  int bytesread = _socket->fetch(&_request);
  if (bytesread <= 0) {
    return 0;
  }
  unsigned loadsize = _request.payload();
  if (!enabled()) {
    if (loadsize) {
      char* eat = new char[loadsize];
      bytesread += _socket->read(eat, loadsize);
      delete [] eat;
    }
    return 1;
  }

  switch (_request.type()) {
  case MonMessage::DescriptionReq:
    description();
    break;
  case MonMessage::PayloadReq:
    payload(loadsize);
    break;
  default:
    //    reply(MonMessage::NoOp,1);
    break;
  }
  return 1;
}

