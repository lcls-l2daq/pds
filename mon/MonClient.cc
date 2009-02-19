#include <string.h>
#include <assert.h>
#include <netdb.h>
#include <netinet/in.h>

#include "pds/mon/MonClient.hh"
#include "pds/mon/MonCds.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntry.hh"
#include "pds/mon/MonDescEntry.hh"
#include "pds/mon/MonConsumerClient.hh"
#include "pds/mon/MonEntryFactory.hh"

using namespace Pds;

MonClient::MonClient(MonConsumerClient& consumer, 
		     MonPort::Type type, 
		     unsigned id,
		     const char* hostname) :
  MonSocket(),
  _id(id),
  _consumer(consumer),
  _cds(type),
  _request(MonMessage::NoOp),
  _reply(MonMessage::NoOp),
  _iovload(0),
  _iovcnt(0),
  _desc(0),
  _descsize(0),
  _maxdescsize(0),
  _usage()
{
  _iovreq[0].iov_base = &_request;
  _iovreq[0].iov_len = sizeof(_request);

  {
    unsigned short port = MonPort::port(type);
    int address = 0;
    hostent* host = gethostbyname(hostname);
    if (host && host->h_addr_list[0]) {
      address = ntohl(*(int*)host->h_addr_list[0]);
    }
    _dst = Ins(address, port);
  }

}

MonClient::~MonClient() 
{
  delete [] _iovload;
  delete [] _desc;
}

int MonClient::use(int signature) {return _usage.use(signature);}
int MonClient::dontuse(int signature) {return _usage.dontuse(signature);}

int MonClient::askdesc()
{
  _usage.reset();
  _request.type(MonMessage::DescriptionReq);
  _request.payload(0);
  return writev(_iovreq, 1);
}

int MonClient::askload()
{
  if (_usage.ismodified()) payload();
  _request.type(MonMessage::PayloadReq);
  _request.payload(_iovreq[1].iov_len);
  return writev(_iovreq, 2);
}

int MonClient::fd() const { return socket(); }

int MonClient::processIo()
{
  int bytesread = read(&_reply, sizeof(_reply));
  if (bytesread <= 0) {
    _consumer.process(*this, MonConsumerClient::Disconnected);
    return 0;
  }
  switch (_reply.type()) {
  case MonMessage::Disabled:
    _consumer.process(*this, MonConsumerClient::Disabled);
    break;
  case MonMessage::Enabled:
    _consumer.process(*this, MonConsumerClient::Enabled);
    break;
  case MonMessage::Description:
    {
      _descsize = _reply.payload();
      adjustdesc();
      bytesread += read(_desc, _descsize);
      description();
      adjustload();
      _consumer.process(*this, MonConsumerClient::Description);
    }
    break;
  case MonMessage::Payload:
    {
      bytesread += readv(_iovload, _usage.used());
      _consumer.process(*this, MonConsumerClient::Payload);
    }
    break;
  default:
    break;
  }
  return 1;
}

void MonClient::adjustload() 
{
  unsigned iovcnt = _cds.totalentries();
  if (iovcnt > _iovcnt) {
    delete [] _iovload;
    _iovload = new iovec[iovcnt];
    _iovcnt = iovcnt;
  }
}

void MonClient::adjustdesc() 
{
  if (_descsize > _maxdescsize) {
    delete [] _desc;
    _desc = new char[_descsize];
    _maxdescsize = _descsize;
  }
}

MonCds& MonClient::cds() {return _cds;}
const MonCds& MonClient::cds() const {return _cds;}
const Ins& MonClient::dst() const {return _dst;}
unsigned MonClient::id() const {return _id;}
bool MonClient::needspayload() const {return _usage.used();}

void MonClient::payload()
{
  for (unsigned short u=0; u<_usage.used(); u++) {
    MonEntry* entry = _cds.entry(_usage.signature(u)); 
    entry->payload(_iovload[u]);
  }
  _usage.request(_iovreq[1]);
}

void MonClient::description()
{
  const char* d = _desc;
  const char* last = d+_descsize;
  if (d < last) {
    const MonDesc* cdsdesc = (const MonDesc*)d;
    assert(cdsdesc->id() == _cds.desc().id());
    _cds.reset();
    d += sizeof(MonDesc);
  }
  while (d < last) {
    const MonDesc* groupdesc = (const MonDesc*)d;
    MonGroup* group = new MonGroup(groupdesc->name());
    unsigned short nentries = groupdesc->nentries();
    _cds.add(group);
    d += sizeof(MonDesc);
    for (unsigned short e=0; e<nentries; e++) {
      const MonDescEntry* entrydesc = (const MonDescEntry*)d;
      MonEntry* entry = MonEntryFactory::entry(*entrydesc);
      group->add(entry);
      d += entrydesc->size();
    }
  }
}
