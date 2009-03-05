#include "ObserverLevel.hh"
#include "EventStreams.hh"
#include "EventCallback.hh"
#include "pds/collection/Message.hh"
#include "pds/utility/Transition.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/InletWire.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/InletWireIns.hh"

#include "pds/service/NetServer.hh"
#include "pds/utility/NetDgServer.hh"

using namespace Pds;

static const unsigned MaxPayload = 2048;

static const unsigned ConnectTimeOut = 250; // 1/4 second

ObserverLevel::ObserverLevel(unsigned platform,
			     EventCallback& callback,
			     Arp* arp) :
  PartitionMember(platform, Level::Observer, arp),
  _callback(callback),
  _streams (0),
  _reply   (Message::Ping)
{
}

ObserverLevel::~ObserverLevel() 
{
  if (_streams) delete _streams;
}

bool ObserverLevel::attach()
{
  start();
  if (connect()) {
    _streams = new EventStreams(*this);
    _streams->connect();
    
    _callback.attached(*_streams);
    
    //    _rivals.insert(_index, msg.reply_to()); // revisit
    Message join(Message::Join);
    mcast(join);
    return true;
  } else {
    _callback.failed(EventCallback::PlatformUnavailable);
    return false;
  }
}

void ObserverLevel::dissolved()
{
  _streams->disconnect();
  _streams->connect();
}

Message& ObserverLevel::reply(Message::Type)
{
  return _reply;
}

void ObserverLevel::allocated(const Allocate& alloc,
			      unsigned        index) 
{
  InletWire* wire = _streams->wire(StreamParams::FrameWork);

  // setup event servers
  unsigned partition  = alloc.partitionid();
  unsigned nnodes     = alloc.nnodes();
  unsigned eventid = 0;
  unsigned controlid = 0;
  for (unsigned n=0; n<nnodes; n++) {
    const Node* node = alloc.node(n);
    if (node->level() == Level::Event) {
      // Add vectored output clients on bld_wire
      Ins ins = StreamPorts::event(partition,
				   Level::Recorder,
				   index,
				   eventid++);
      printf("ObserverLevel::allocated joining segment from %08x to port %x/%d\n",
	     node->ip(),ins.address(),ins.portId());
      
      NetDgServer* srv = new NetDgServer(ins,
					 node->procInfo(),
					 EventStreams::netbufdepth*EventStreams::MaxSize);
      wire->add_input(srv);
      Ins mcastIns(ins.address());
      srv->server().join(mcastIns, Ins(header().ip()));
      printf("ObserverLevel::allocated assign fragment %d  %x/%d\n",
	     srv->id(),mcastIns.address(),srv->server().portId());
    }
  }
}

void ObserverLevel::post     (const Transition& tr)
{
  InletWire* wire = _streams->wire(StreamParams::FrameWork);
  wire->post(tr);
}

void ObserverLevel::post     (const InDatagram& in)
{
  InletWire* wire = _streams->wire(StreamParams::FrameWork);
  wire->post(in);
}

void ObserverLevel::detach()
{
  if (_streams) {
    _streams->disconnect();
    delete _streams;
    _streams = 0;
    //    _callback.dissolved(_dissolver);
  }
  cancel();
}
