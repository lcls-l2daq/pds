#include <string.h>
#include <assert.h>
#include <errno.h>

#include "pds/mon/MonClient.hh"
#include "pds/mon/MonCds.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntry.hh"
#include "pds/mon/MonDescEntry.hh"
#include "pds/mon/MonConsumerClient.hh"
#include "pds/mon/MonEntryFactory.hh"
#include "pds/utility/Mtu.hh"

#include <stdlib.h>

using namespace Pds;

MonClient::MonClient(MonConsumerClient& consumer, 
		     MonCds* cds,
		     MonSocket& socket,
		     const Src& src) :
  _src    (src),
  _consumer(consumer),
  _cds    (cds),
  _socket (socket),
  _request(MonMessage::NoOp),
  _reply  (MonMessage::NoOp),
  _iovload(0),
  _iovcnt (0),
  _desc   (0),
  _descsize(0),
  _maxdescsize(0),
  _usage()
{
  _iovreq[0].iov_base = &_request;
  _iovreq[0].iov_len = sizeof(_request);
}

MonClient::~MonClient() 
{
  delete _cds;
  delete [] _iovload;
  delete [] _desc;
}

MonSocket& MonClient::socket() { return _socket; }

int MonClient::use(int signature) {return _usage.use(signature);}
int MonClient::dontuse(int signature) {return _usage.dontuse(signature);}
int MonClient::use_all() 
{
  unsigned used = 0;
  for (unsigned short g=0; g<_cds->ngroups(); g++) {
    const MonGroup* group = _cds->group(g);
    for (unsigned short e=0; e<group->nentries(); e++, used++)
      _usage.use(group->entry(e)->desc().signature());
  }
  payload();
  return used;
}

const Src& MonClient::src() const { return _src; }

int MonClient::askdesc()
{
  _usage.reset();
  _request.type(MonMessage::DescriptionReq);
  _request.payload(0);
  return _socket.writev(_iovreq, 1);
}

int MonClient::askload()
{
  if (_usage.ismodified()) payload();
  _request.type(MonMessage::PayloadReq);
  _request.payload(_iovreq[1].iov_len);
  return _socket.writev(_iovreq, 2);
}

int MonClient::fd() const { return _socket.socket(); }

#include <stdio.h>

int MonClient::processIo()
{
  int bytesread = _socket.read(&_reply, sizeof(_reply));
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
    read_description(_reply.payload());
    break;
  case MonMessage::Payload:
    read_payload();
    break;
  default:
    break;
  }
  return 1;
}

void MonClient::adjustload() 
{
  unsigned iovcnt = _cds->totalentries();
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

MonCds& MonClient::cds() {return *_cds;}
const MonCds& MonClient::cds() const {return *_cds;}
const Ins& MonClient::dst() const {return _dst;}
void MonClient::dst(const Ins& d) { _dst=d; }
bool MonClient::needspayload() const {return _usage.used();}

void MonClient::payload()
{
  for (unsigned short u=0; u<_usage.used(); u++) {
    MonEntry* entry = _cds->entry(_usage.signature(u)); 
    entry->payload(_iovload[u]);
  }
  _usage.request(_iovreq[1]);
}

void MonClient::read_payload()
{
  int bytes = _socket.readv(_iovload, _usage.used());
  if (bytes >= 0)
    ;
  else
    printf("payload error : reason %s\n", strerror(errno));
  _consumer.process(*this, MonConsumerClient::Payload);
}

void MonClient::read_description(int descsize)
{
  _descsize = descsize;
  adjustdesc();
  _socket.read(_desc, _descsize);

  unsigned payload_sz=0;
  const char* d = _desc;
  const char* last = d+_descsize;
  if (d < last) {
    const MonDesc* cdsdesc = (const MonDesc*)d;
    assert(cdsdesc->id() == _cds->desc().id());
    _cds->reset();
    d += sizeof(MonDesc);
  }
  while (d < last) {
    const MonDesc* groupdesc = (const MonDesc*)d;
    MonGroup* group = new MonGroup(groupdesc->name());
    unsigned short nentries = groupdesc->nentries();
    _cds->add(group);
    d += sizeof(MonDesc);
    for (unsigned short e=0; e<nentries; e++) {
      const MonDescEntry* entrydesc = (const MonDescEntry*)d;
      MonEntry* entry = MonEntryFactory::entry(*entrydesc);
      group->add(entry);
      iovec t; entry->payload(t); payload_sz += t.iov_len;
      d += entrydesc->size();
    }
  }

  adjustload();
  
  _consumer.process(*this, MonConsumerClient::Description);

  if (payload_sz>Mtu::Size) {
    printf("MonClient payload size %d > max (%d).  Aborting\n",
	   payload_sz, Mtu::Size);
    abort();
  }
}
