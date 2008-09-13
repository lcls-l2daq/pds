#include "ControlStreams.hh"
#include "ControlSourceWire.hh"
#include "pds/utility/Eb.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/BitList.hh"
#include "pds/collection/Node.hh"
#include "pds/service/Task.hh"

class ControlOccHandler: public Appliance {
public:
  ControlOccHandler(ControlStreams& streams) : 
    _streams(streams),
    _synchs(sizeof(Occurrence), 1)
  {}

  virtual Datagram* occurrences(Datagram* in)
  {
    Occurrence* occ = (Occurrence*) in;
    switch (occ->type()) {
    case OccurrenceId::TrimRequest:
      _streams.trim_input(occ->reason());
      return 0;
    case OccurrenceId::SynchRequest:
      return new(&_synchs) Occurrence(OccurrenceId::Synch, 0);
    case OccurrenceId::Reboot:
    case OccurrenceId::Vmon:
      return in;
    default:
      return 0;
    }
  }
  virtual Datagram* transitions(Datagram* in) {return 0;}

private:
  ControlStreams& _streams;
  GenericPool _synchs;
};

static const unsigned ServerDatagramSize = 0;
static const unsigned ServerPayloadSize = sizeof(Occurrence);
static const unsigned ServerMaxDatagrams = 32;

class CtrloopServer : public Server {
public:
  CtrloopServer(unsigned id, int ipaddress, InletWire& inlet_wire) : 
    Server(id, Ins(ipaddress), ServerDatagramSize, 
	      ServerPayloadSize, ServerMaxDatagrams),
    _inlet_wire(inlet_wire)
  {}
  virtual ~CtrloopServer() {}

private: 
  // Implements Server
  char* payload() {return _payload;}

  int commit(char* datagram, char* payload, int size, const Ins& src)
  {
    const Datagram& input = *(Datagram*)payload;
    _inlet_wire.post(input);
    return 1;
  }

  int handleError(int value)
  {
    printf("*** CtrloopServer error: %s\n", strerror(value));
    return 1;
  }

private: 
  InletWire& _inlet_wire;
  char _payload[ServerPayloadSize];
};

static const unsigned DatagramSize = sizeof(int);
static const unsigned PayloadSize = 0;
static const unsigned TimeOut = 0;
static const unsigned MaxDatagrams = 1;
static const int TaskPriority = 60;
static const char* TaskName = "oCtrloop";

class CtrloopManager : public ServerManager {
public:
  CtrloopManager(int ipaddress, InletWire** inlet_wires) : 
    ServerManager(Ins(ipaddress), DatagramSize, PayloadSize, 
		     TimeOut, MaxDatagrams),
    _task(new Task(TaskObject(TaskName, TaskPriority))),
    _driver(*this),
    _sem(Semaphore::EMPTY)
  {
    donottimeout();
    for (unsigned s = 0; s < StreamParams::NumberOfStreams; s++) {
      _servers[s] = new CtrloopServer(s, ipaddress, *inlet_wires[s]);
      manage(_servers[s]);
      _ctrloop.port(s, _servers[s]->portId());
    }
    arm();
  }

  virtual ~CtrloopManager()
  {
    for (unsigned s = 0; s < StreamParams::NumberOfStreams; s++) {
      delete _servers[s];
    }
    _task->destroy();
  }

  void connect() 
  {
    _driver.run(*_task);
  }

  void disconnect() 
  {
    unblock((char*)this, 0, 0);
    _sem.take();
  }

  const StreamPorts& ctrloop() const {return _ctrloop;}

private:
  // Implements Server
  virtual int commit(char* dg, char* payload, int size, const Ins& src)
  {
    if (!size) {
      _sem.give();
      return 0;
    }
    return 1;
  }

  virtual int handleError(int value)
  {
    printf("*** CtrloopManager error: %s\n", strerror(value));
    return 1;
  }
  virtual char* payload() {return 0;}

  // Implements ServerManager
  virtual int processIo(Server* server) {return server->pend(MSG_DONTWAIT);}
  virtual int processTmo() {return 1;}

private:
  StreamPorts _ctrloop;
  Server* _servers[StreamParams::NumberOfStreams];
  Task* _task;
  SelectDriver _driver;
  Semaphore _sem;
};

ControlStreams::ControlStreams(const Node& node) :
  WiredStreams(VmonSourceId(VmonSourceId::Control))
{
  const unsigned MaxSlotsPerCrate = 10;

  VmonEb vmoneb(vmon());
  unsigned crates = node.crates();
  int ipaddress = node.ip();
  unsigned id = node.id();
  unsigned ncrates = BitList::count(crates);
  unsigned nroms = MaxSlotsPerCrate*ncrates;
  unsigned eventpooldepth = 32;
  unsigned netbufdepth = 32;
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    StreamParams::StreamType type = StreamParams::StreamType(s);
    EbTimeouts ebtimeouts(crates, type, Level::Control);
    _outlets[s] = new ControlSourceWire(*stream(s)->outlet(), s);
    const unsigned safe = 2; // Try to keep into account possible user payload
    unsigned eventsize = (s == StreamParams::FrameWork ?
      sizeof(Datagram)*safe*EbBitMask::BitMaskBits*(ncrates+nroms) :
      sizeof(Occurrence)*EbBitMask::BitMaskBits*(ncrates+nroms));
    _inlet_wires[s] = 
      new Eb(Src(id), Level::Control, *stream(s)->inlet(),
		*_outlets[s], s, ipaddress,
		eventsize, eventpooldepth, netbufdepth,
		ebtimeouts, vmoneb);
  }
  _occ_handler = new ControlOccHandler(*this);
  _occ_handler->connect(stream(StreamParams::Occurrence)->inlet());
  _ctrloop_manager = new CtrloopManager(ipaddress, _inlet_wires);
  _ctrloop_manager->connect();
}

ControlStreams::~ControlStreams() 
{
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    delete _inlet_wires[s];
    delete _outlets[s];
  }
  _ctrloop_manager->disconnect();
  delete _ctrloop_manager;
  delete _occ_handler;
}

void ControlStreams::reboot() 
{
  Occurrence reboot(OccurrenceId::Reboot, 0);
  _inlet_wires[StreamParams::Occurrence]->post(reboot);
}

const StreamPorts& ControlStreams::ctrloop() const
{
  return _ctrloop_manager->ctrloop();
}
