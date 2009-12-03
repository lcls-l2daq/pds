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


MonServer::MonServer(const Src& src,
		     const MonCds& cds, 
		     MonUsage& usage, 
		     MonSocket& socket) :
  _cds     (cds),
  _usage   (usage),
  _socket  (socket),
  _reply   (src,MonMessage::NoOp),
  _signatures(0),
  _sigcnt (0),
  _enabled(true)
{
  _iovreply = new iovec[1];
  _iovreply[0].iov_base = &_reply;
  _iovreply[0].iov_len = sizeof(_reply);
  _iovcnt = 1;
}

MonServer::~MonServer() 
{
  delete [] _iovreply;
  delete [] _signatures;
}

MonSocket& MonServer::socket() { return _socket; }

void MonServer::enable() 
{
  _reply.type(MonMessage::Enabled);
  _reply.payload(0);
  if (_socket.writev(_iovreply, 1) < 0) {
    printf("*** MonServer::enable send error socket [%d]: %s\n", 
	   _socket.socket(), strerror(errno));
  }
  _enabled = true;
}

void MonServer::disable() 
{
  _reply.type(MonMessage::Disabled);
  _reply.payload(0);
  if (_socket.writev(_iovreply, 1) < 0) {
    printf("*** MonServer::disable send error socket [%d]: %s\n", 
	   _socket.socket(), strerror(errno));
  }
  _enabled = false;
}

bool MonServer::enabled() const { return _enabled; }

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

void MonServer::reply(MonMessage::Type type,int cnt)
{
  _reply.type(type);
  _reply.payload(_iovreply+1, cnt-1);
  _cds.payload_sem().take();
  int bytessent = _socket.writev(_iovreply, cnt);
  _cds.payload_sem().give();
  if (bytessent < 0) {
    printf("*** MonServer::reply send error socket [%d]: %s\n", 
	   _socket.socket(), strerror(errno));
  }
}

void MonServer::description()
{
  adjust();

  iovec* iov = _iovreply+1;
  unsigned element = _cds.description(iov);

  reply(MonMessage::Description,element+1);
}

void MonServer::payload()
{
  adjust();

  unsigned used = 0;
  iovec* iov = _iovreply+1;
  for (unsigned short g=0; g<_cds.ngroups(); g++) {
    const MonGroup* group = _cds.group(g);
    for (unsigned short e=0; e<group->nentries(); e++, iov++, used++)
      group->entry(e)->payload(*iov); 
  }

  reply(MonMessage::Payload,used+1);
}

void MonServer::payload(unsigned loadsize)
{
  adjust();

  _socket.read(_signatures, loadsize);

  unsigned used = loadsize>>2;
  iovec* iov = _iovreply+1;
  const int* signatures = _signatures;
  for (unsigned u=0; u<used; u++, signatures++, iov++) {
    const MonEntry* entry = _cds.entry(*signatures); 
    if (entry) {
      entry->payload(*iov);
      _usage.use(*signatures);
    }
  }

  reply(MonMessage::Payload,used+1);
}

