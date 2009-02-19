#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "pds/mon/MonServer.hh"
#include "pds/mon/MonCds.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntry.hh"
#include "pds/mon/MonDescEntry.hh"
#include "pds/mon/MonUsage.hh"
#include "pds/service/Semaphore.hh"

using namespace Pds;


MonServer::MonServer(const MonCds& cds, 
		     MonUsage& usage, 
		     int socket) :
  MonSocket(socket),
  _cds(cds),
  _usage(usage),
  _request(MonMessage::NoOp),
  _reply(MonMessage::NoOp),
  _iovreply(0),
  _iovcnt(0),
  _signatures(0),
  _sigcnt(0),
  _enabled(true)
{
}

MonServer::~MonServer() 
{
  delete [] _iovreply;
  delete [] _signatures;
}

void MonServer::enable() 
{
  _reply.type(MonMessage::Enabled);
  _reply.payload(0);
  if (writev(_iovreply, 1) < 0) {
    printf("*** MonServer::enable send error socket [%d]: %s\n", 
	   socket(), strerror(errno));
  }
  _enabled = true;
}

void MonServer::disable() 
{
  _reply.type(MonMessage::Disabled);
  _reply.payload(0);
  if (writev(_iovreply, 1) < 0) {
    printf("*** MonServer::disable send error socket [%d]: %s\n", 
	   socket(), strerror(errno));
  }
  _enabled = false;
}

int MonServer::fd() const { return socket(); }

int MonServer::processIo()
{
  int bytesread = read(&_request, sizeof(_request));
  if (bytesread <= 0) {
    return 0;
  }
  unsigned loadsize = _request.payload();
  if (!_enabled) {
    if (loadsize) {
      char* eat = new char[loadsize];
      bytesread += read(eat, loadsize);
      delete [] eat;
    }
    return 1;
  }
  unsigned cnt = 1;
  switch (_request.type()) {
  case MonMessage::DescriptionReq:
    {
      adjust();
      cnt = description()+1;
      _reply.type(MonMessage::Description);
    }
    break;
  case MonMessage::PayloadReq:
    {
      bytesread += read(_signatures, loadsize);
      cnt = payload(loadsize>>2)+1;
      _reply.type(MonMessage::Payload);
    }
    break;
  default:
    {
      _reply.type(MonMessage::NoOp);
    }
    break;
  }
  _reply.payload(_iovreply+1, cnt-1);
  _cds.payload_sem().take();
  int bytessent = writev(_iovreply, cnt);
  _cds.payload_sem().give();
  if (bytessent < 0) {
    printf("*** MonServer::processIo send error socket [%d]: %s\n", 
	   socket(), strerror(errno));
  }
  return 1;
}

void MonServer::adjust() 
{
  unsigned iovcnt = _cds.totaldescs();
  if (iovcnt > _iovcnt) {
    delete [] _iovreply;
    _iovreply = new iovec[iovcnt+1];
    _iovreply[0].iov_base = &_reply;
    _iovreply[0].iov_len = sizeof(_reply);
    _iovcnt = iovcnt;
  }
  unsigned sigcnt = _cds.totalentries();
  if (sigcnt > _sigcnt) {
    delete [] _signatures;
    _signatures = new int[sigcnt];
    _sigcnt = sigcnt;
  }
}  

unsigned MonServer::description()
{
  unsigned element=0;
  iovec* iov = _iovreply+1;
  iov[element].iov_base = (void*)&_cds.desc();
  iov[element].iov_len = sizeof(MonDesc);
  element++;
  for (unsigned short g=0; g<_cds.ngroups(); g++) {
    const MonGroup* group = _cds.group(g);
    iov[element].iov_base = (void*)&group->desc();
    iov[element].iov_len = sizeof(MonDesc);
    element++;
    for (unsigned short e=0; e<group->nentries(); e++) {
      const MonEntry* entry = group->entry(e); 
      iov[element].iov_base = (void*)&entry->desc();
      iov[element].iov_len = entry->desc().size();
      element++;
    }
  }
  return element;
}

unsigned MonServer::payload(unsigned used)
{
  iovec* iov = _iovreply+1;
  const int* signatures = _signatures;
  for (unsigned u=0; u<used; u++, signatures++, iov++) {
    const MonEntry* entry = _cds.entry(*signatures); 
    entry->payload(*iov);
    _usage.use(*signatures);
  }
  return used;
}

