#include "InletWireServer.hh"
//#include "Stream.hh"
#include "OutletWire.hh"
#include "Inlet.hh"
#include "InletWireIns.hh"
#include "pds/service/Task.hh"
#include "pds/collection/Transition.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/CDatagram.hh"

using namespace Pds;

static const int AddInput     = 1;
static const int RemoveInput  = 2;
static const int AddOutput    = 3;
static const int RemoveOutput = 4;
static const int TrimInput    = 5;
static const int TrimOutput   = 6;
static const int PostDatagram = 7;
static const int PostTransition = 8;

static const unsigned DatagramSize = sizeof(int);
static const unsigned PayloadSize = sizeof(InletWireIns) > sizeof(CDatagram) ? sizeof(InletWireIns) : sizeof(CDatagram);
static const unsigned MaxDatagrams = 64;

InletWireServer::InletWireServer(Inlet& inlet,
				 OutletWire& outlet,
				 int ipaddress,
				 int stream, 
				 int taskpriority,
				 const char* taskname,
				 unsigned timeout) :
  InletWire(taskpriority, taskname),
  ServerManager(Ins(ipaddress), DatagramSize, PayloadSize, 
		timeout, MaxDatagrams),
  _inlet(inlet),
  _outlet(outlet),
  //  _stream(StreamParams::StreamType(stream)),
  _ipaddress(ipaddress),
  _payload(new char[PayloadSize]),
  _transition(new char[sizeof(Transition)]),
  _datagrams(sizeof(CDatagram), MaxDatagrams),
  _driver(*this),
  _sem(Semaphore::EMPTY)
{
  if (!timeout) donottimeout();
}

InletWireServer::~InletWireServer() 
{
  delete [] _transition;
  delete [] _payload;
}

void InletWireServer::connect() 
{
  _driver.run(task());
}

void InletWireServer::disconnect() 
{
  unblock((char*)this, 0, 0);
  _sem.take();
}

void InletWireServer::add_input(Server* srv)
{
  unblock((char*)&AddInput, (char*)&srv, sizeof(srv));
  _sem.take();
}

void InletWireServer::remove_input(Server* srv)
{
  unblock((char*)&RemoveInput, (char*)&srv, sizeof(srv));
  _sem.take();
}

void InletWireServer::add_output(const InletWireIns& iwi) 
{
  unblock((char*)&AddOutput, (char*)&iwi, sizeof(iwi));
  _sem.take();
}

void InletWireServer::remove_output(const InletWireIns& iwi)
{
  unblock((char*)&RemoveOutput, (char*)&iwi, sizeof(iwi));
  _sem.take();
}

void InletWireServer::trim_input(Server* srv)
{
  if (task().is_self()) {
    _remove_input(srv);
  } else {
    unblock((char*)&TrimInput, (char*)&srv, sizeof(srv));
    _sem.take();
  }
}

void InletWireServer::trim_output(const InletWireIns& iwi)
{
  if (task().is_self()) {
    remove_output(iwi.id());
  } else {
    unblock((char*)&TrimOutput, (char*)&iwi, sizeof(iwi));
  }
}

void InletWireServer::post(const Transition& tr)
{
  unblock((char*)&PostTransition, (char*)&tr, sizeof(Transition));
}

void InletWireServer::post(const Datagram& dg)
{
  unblock((char*)&PostDatagram, (char*)&dg, sizeof(Datagram));
}

unsigned short InletWireServer::port(unsigned id) const
{
  return _ports[id];
}

char* InletWireServer::payload() 
{
  return _payload;
}

int InletWireServer::commit(char* datagram, 
			    char* payload, 
			    int sizeofPayload, 
			    const Ins& src)
{
  if (!sizeofPayload) {
    unsigned id;
    for (id=0; id<EbBitMaskArray::BitMaskBits; id++) {
      if (_ports[id]) remove_input(server(id));
    }
    for (id=0; id<EbBitMaskArray::BitMaskBits; id++) {
      if (_outputs.hasBitSet(id)) remove_output(id);
    }
    flush();
    _sem.give();
    return 0;
  }
  int request = *(int*)datagram;
  const InletWireIns& iwi = *(const InletWireIns*)payload;
  Server* srv = *reinterpret_cast<Server**>(payload);
  switch (request) {
  case AddInput:
    _add_input(srv);
    _sem.give();
    break;
  case RemoveInput:
    _remove_input(srv);
    _sem.give();
    break;
  case AddOutput:
    add_output(iwi.id(), iwi.rcvr(), iwi.mcast());
    _sem.give();
    break;
  case RemoveOutput:
    remove_output(iwi.id());
    _sem.give();
    break;
  case TrimInput:
    _remove_input(srv);
    break;
  case TrimOutput:
    remove_output(iwi.id());
    break;
  case PostDatagram:
    {
      CDatagram* dg = new(&_datagrams) CDatagram(*(Datagram*)payload);
      _inlet.post(dg);
    }
    break;
  case PostTransition:
    {
      memcpy(_transition,payload,sizeof(Transition));
      Transition* tr = reinterpret_cast<Transition*>(_transition);
      _inlet.post(tr);
    }
  }
  return 1;
}

void InletWireServer::_add_input(Server* srv)
{
  if (!accept(srv)) {
    printf("InletWireServer::_add_input error in accept %p\n", srv);
  }
}

void InletWireServer::_remove_input(Server* srv)
{
  remove(srv->id());
}

void InletWireServer::add_output(unsigned id, const Ins& rcvr, int mcast)
{
  _outlet.bind(id, rcvr, mcast);
  _outputs.setBit(id);  
}

void InletWireServer::remove_output(unsigned id)
{
  _outlet.unbind(id);
  _outputs.clearBit(id);
}


int InletWireServer::handleError(int value)
{
  printf("*** InletWireServer error: %s\n", strerror(value));
  return 1;
}

int InletWireServer::processIo(Server* srv) 
{
  printf("InletWireServer::processIo srvId %d\n",srv->id());
  return srv->pend(MSG_DONTWAIT);
}

int InletWireServer::processTmo() 
{
  return 1;
}
