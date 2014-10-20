#include "InletWireServer.hh"
//#include "Stream.hh"
#include "OutletWire.hh"
#include "Inlet.hh"
#include "InletWireIns.hh"
#include "Transition.hh"
#include "Occurrence.hh"
#include "pds/service/Task.hh"
#include "pds/utility/Mtu.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/CDatagram.hh"

using namespace Pds;

static const int AddInput     = 1;
static const int RemoveInput  = 2;
static const int AddOutput    = 3;
static const int RemoveOutput = 4;
static const int TrimInput    = 5;
static const int TrimOutput   = 6;
static const int PostInDatagram = 7;  // Assumed to be a "local" out-of-band message, we may pass pointers
static const int PostTransition = 8;  // Ditto.
static const int PostOccurrence = 9;  // Ditto.
static const int RemoveOutputs = 10;
static const int FlushInputs   = 11;
static const int FlushOutputs  = 12;

static const unsigned DatagramSize = sizeof(int); // out-of-band message header size
static const unsigned PayloadSize  = sizeof(Transition);
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
  _ipaddress(ipaddress),
  _payload(new char[PayloadSize]),
  _driver(*this),
  _sem(Semaphore::EMPTY)
{
  if (!timeout) donottimeout();
}

InletWireServer::~InletWireServer()
{
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

void InletWireServer::add_input_nonblocking(Server* srv)
{
  if (task().is_self()) {
    _add_input(srv);
  } else {
    unblock((char*)&AddInput, (char*)&srv, sizeof(srv));
    _sem.take();
  }
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

void InletWireServer::remove_outputs()
{
  unblock((char*)&RemoveOutputs, (char*)this, sizeof(this)); // dummy arguments
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

void InletWireServer::flush_inputs()
{
  unblock((char*)&FlushInputs, (char*)this, sizeof(this));
  _sem.take();
}

void InletWireServer::flush_outputs()
{
  unblock((char*)&FlushOutputs, (char*)this, sizeof(this));
  _sem.take();
}

void InletWireServer::post(const Transition& tr)
{
  // assume that this object is constructed from a pool, and control has been handed over.
  Transition* ptr = const_cast<Transition*>(&tr);
  unblock((char*)&PostTransition, (char*)&ptr, sizeof(ptr));
}

void InletWireServer::post(const Occurrence& tr)
{
  // assume that this object is constructed from a pool, and control has been handed over.
  Occurrence* ptr = const_cast<Occurrence*>(&tr);
  unblock((char*)&PostOccurrence, (char*)&ptr, sizeof(ptr));
}

void InletWireServer::post(const InDatagram& dg)
{
  // assume that this object is constructed from a pool, and control has been handed over.
  InDatagram* ptr = const_cast<InDatagram*>(&dg);
  unblock((char*)&PostInDatagram, (char*)&ptr, sizeof(ptr));
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
    int id;
    EbBitMask inputs(managed());
    for (id=0; id<EbBitMaskArray::BitMaskBits; id++) {
      if ( inputs.hasBitSet(id)) remove(id);
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
  Server* srv    = *reinterpret_cast<Server**>(payload);
  InDatagram* dg = *reinterpret_cast<InDatagram**>(payload);
  Transition* tr = *reinterpret_cast<Transition**>(payload);
  Occurrence* occ= *reinterpret_cast<Occurrence**>(payload);
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
    add_output(iwi.id(), iwi.rcvr());
    _sem.give();
    break;
  case RemoveOutput:
    remove_output(iwi.id());
    _sem.give();
    break;
  case RemoveOutputs:
    for (int id=0; id<EbBitMaskArray::BitMaskBits; id++)
      if (_outputs.hasBitSet(id)) remove_output(id);
    _sem.give();
    break;
  case TrimInput:
    _remove_input(srv);
    break;
  case TrimOutput:
    remove_output(iwi.id());
    break;
  case FlushInputs:
    _flush_inputs();
    _sem.give();
    break;
  case FlushOutputs:
    _flush_outputs();
    _sem.give();
    break;
  case PostInDatagram:
    _inlet.post(dg);
    break;
  case PostTransition:
    _inlet.post(tr);
    break;
  case PostOccurrence:
    _inlet.post(occ);
    break;
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

void InletWireServer::add_output(unsigned id, const Ins& rcvr)
{
  _outlet.bind(id, rcvr);
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

void InletWireServer::_flush_inputs()
{
}

void InletWireServer::_flush_outputs()
{
}
