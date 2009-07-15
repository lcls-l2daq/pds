#include "RecorderLevel.hh"
#include "EventStreams.hh"
#include "EventCallback.hh"
#include "pds/collection/Message.hh"
#include "pds/utility/Transition.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/OutletWire.hh"
#include "pds/utility/InletWire.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/InletWireIns.hh"

#include "pds/service/NetServer.hh"
#include "pds/utility/BldServer.hh"
#include "pds/utility/NetDgServer.hh"
#include "pds/management/MsgAppliance.hh"
#include "pds/vmon/VmonEb.hh"

using namespace Pds;

static const unsigned MaxPayload = 2048;

static const unsigned ConnectTimeOut = 250; // 1/4 second

RecorderLevel::RecorderLevel(unsigned platform,
			     EventCallback& callback,
			     Arp* arp) :
  PartitionMember(platform, Level::Recorder, arp),
  _callback(callback),
  _streams (0),
  _reply   (Message::Ping)
{
}

RecorderLevel::~RecorderLevel() 
{
  if (_streams) delete _streams;
}

bool RecorderLevel::attach()
{
  start();
  if (connect()) {
    _streams = new EventStreams(*this,
				new VmonEb(header().procInfo(),32,16,(1<<24),(1<<22)));
    _streams->connect();
    
    _callback.attached(*_streams);

    (new MsgAppliance)->connect(_streams->stream()->inlet());
    
    //    _rivals.insert(_index, msg.reply_to()); // revisit
    Message join(Message::Join);
    mcast(join);
    return true;
  } else {
    _callback.failed(EventCallback::PlatformUnavailable);
    return false;
  }
}

void RecorderLevel::dissolved()
{
  _streams->disconnect();
  _streams->connect();
}

Message& RecorderLevel::reply(Message::Type)
{
  return _reply;
}

void RecorderLevel::allocated(const Allocation& alloc,
			      unsigned          index) 
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
      printf("RecorderLevel::allocated joining segment from %08x to port %x/%d\n",
	     node->ip(),ins.address(),ins.portId());
      
      Ins srvIns(ins.portId());
      NetDgServer* srv = new NetDgServer(srvIns,
					 node->procInfo(),
					 EventStreams::netbufdepth*EventStreams::MaxSize);
      wire->add_input(srv);
      Ins mcastIns(ins.address());
      srv->server().join(mcastIns, Ins(header().ip()));
      Ins bcastIns = StreamPorts::bcast(partition, Level::Recorder);
      srv->server().join(bcastIns, Ins(header().ip()));
      printf("RecorderLevel::allocated assign fragment %d  %x/%d\n",
	     srv->id(),mcastIns.address(),srv->server().portId());
    }
    else if (node->level() == Level::Control) {
      if (controlid) {
	printf("RecorderLevel::allocated  Additional control level found in allocation\n");
	continue;
      }
      Ins ins = StreamPorts::event(partition,
				   Level::Control,
				   controlid,
				   index);
      InletWireIns wireIns(controlid, ins);
      wire->add_output(wireIns);
      printf("EventLevel::message adding output %d to %x/%d\n",
	     controlid, ins.address(), ins.portId());
      controlid++;
    }
  }
  OutletWire* owire = _streams->stream(StreamParams::FrameWork)->outlet()->wire();
  owire->bind(OutletWire::Bcast, StreamPorts::bcast(partition, 
						    Level::Control,
						    index));
}

void RecorderLevel::post     (const Transition& tr)
{
  InletWire* wire = _streams->wire(StreamParams::FrameWork);
  wire->post(tr);
}

void RecorderLevel::post     (const Occurrence& tr)
{
  InletWire* wire = _streams->wire(StreamParams::FrameWork);
  wire->post(tr);
}

void RecorderLevel::post     (const InDatagram& in)
{
  InletWire* wire = _streams->wire(StreamParams::FrameWork);
  wire->post(in);
}

void RecorderLevel::detach()
{
  if (_streams) {
    _streams->disconnect();
    delete _streams;
    _streams = 0;
    //    _callback.dissolved(_dissolver);
  }
  cancel();
}
