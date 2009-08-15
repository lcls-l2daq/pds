#include "EventLevel.hh"
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
#include "pds/xtc/CDatagram.hh"
#include "pds/management/EbIStream.hh"

using namespace Pds;


static inline bool _is_bld(const Node& n)
{
  return n.level() == Level::Reporter;
}

static inline Ins _bld_ins(const Node& n)
{
  return Ins(n.ip(), StreamPorts::bld(0).portId());
}

static inline Src _bld_src(const Node& n)
{
  return n.procInfo();
}

EventLevel::EventLevel(unsigned platform,
		       EventCallback& callback,
		       Arp* arp) :
  PartitionMember(platform, Level::Event, arp),
  _callback   (callback),
  _streams    (0),
  _inlet      (0),
  _reply      (Message::Ping)
{
}

EventLevel::~EventLevel() 
{
  if (_streams) delete _streams;
  if (_inlet  ) delete _inlet;
}

bool EventLevel::attach()
{
  start();
  if (connect()) {
    _streams = new EventStreams(*this);
    _streams->connect();

    _inlet = new EbIStream(header().procInfo(),
                           header().ip(),
                           Level::Event,
                           *_streams->wire(StreamParams::FrameWork));
    _inlet->input()->connect();
    _streams->wire(StreamParams::FrameWork)->add_input(_inlet->output());

    _callback.attached(*_streams);

    Message join(Message::Join);
    mcast(join);
    return true;
  } else {
    _callback.failed(EventCallback::PlatformUnavailable);
    return false;
  }
}

void EventLevel::dissolved()
{
  // detach the ToEb
  InletWireServer* wire = 
    dynamic_cast<InletWireServer*>(_streams->wire(StreamParams::FrameWork));
  wire->unmanage(wire->server(0));

  // destroy all inputs and outputs
  _inlet->input()->disconnect();
  _streams->disconnect();

  // reattach the ToEb
  _streams->connect();
  _inlet->input()->connect();
  wire->add_input(_inlet->output());
}

void EventLevel::detach()
{
  if (_streams) {
    _streams->disconnect();
    _inlet->input()->disconnect();
    delete _streams;
    delete _inlet;
    _streams = 0;
    _inlet   = 0;
    //    _callback.dissolved(_dissolver);
  }
  cancel();
}

Message& EventLevel::reply(Message::Type)
{
  return _reply;
}

void    EventLevel::allocated(const Allocation& alloc,
			      unsigned          index) 
{
  InletWire* bld_wire = _streams->wire(StreamParams::FrameWork);
  InletWire* pre_wire = _inlet ? _inlet->input() : bld_wire;

  // setup BLD and event servers
  unsigned partition  = alloc.partitionid();
  unsigned nnodes     = alloc.nnodes();
  unsigned recorderid = 0;
  unsigned segmentid  = 0;
  for (unsigned n=0; n<nnodes; n++) {
    const Node& node = *alloc.node(n);
    if (_is_bld(node)) {
      Ins ins( _bld_ins(node) );
      BldServer* srv = new BldServer(ins, _bld_src(node), EventStreams::netbufdepth);
      bld_wire->add_input(srv);
      srv->server().join(ins, Ins(header().ip()));
      printf("EventLevel::allocated assign bld  fragment %d  %x/%d\n",
	     srv->id(),ins.address(),srv->server().portId());
    }
    else if (node.level() == Level::Segment) {
      // Add vectored output clients on bld_wire
      Ins ins = StreamPorts::event(partition,
				   Level::Event,
				   index,
				   segmentid++);
      
      Ins srvIns(ins.portId());
      NetDgServer* srv = new NetDgServer(srvIns,
					 node.procInfo(),
					 EventStreams::netbufdepth*EventStreams::MaxSize);
      pre_wire->add_input(srv);
      Ins mcastIns(ins.address());
      srv->server().join(mcastIns, Ins(header().ip()));
      Ins bcastIns = StreamPorts::bcast(partition, Level::Event);
      srv->server().join(bcastIns, Ins(header().ip()));
      printf("EventLevel::allocated assign fragment %d  %x/%d\n",
	     srv->id(),mcastIns.address(),srv->server().portId());
    }
    else if (node.level() == Level::Recorder) {
	Ins ins = StreamPorts::event(partition,
				     Level::Recorder,
				     recorderid,
				     index);
	InletWireIns wireIns(recorderid, ins);
	bld_wire->add_output(wireIns);
	printf("EventLevel::message adding output %d to %x/%d\n",
	       recorderid, ins.address(), ins.portId());
	recorderid++;
    }
  }
  OutletWire* owire = _streams->stream(StreamParams::FrameWork)->outlet()->wire();
  owire->bind(OutletWire::Bcast, StreamPorts::bcast(partition, 
						    Level::Recorder,
						    index));
}

void    EventLevel::post     (const Transition& tr)
{
  InletWire* bld_wire = _streams->wire(StreamParams::FrameWork);
  InletWire* pre_wire = _inlet ? _inlet->input() : bld_wire;
  pre_wire->post(tr);
}

void    EventLevel::post     (const Occurrence& tr)
{
  InletWire* bld_wire = _streams->wire(StreamParams::FrameWork);
  InletWire* pre_wire = _inlet ? _inlet->input() : bld_wire;
  pre_wire->post(tr);
}

void    EventLevel::post     (const InDatagram& in)
{
  InletWire* bld_wire = _streams->wire(StreamParams::FrameWork);
  InletWire* pre_wire = _inlet ? _inlet->input() : bld_wire;
  pre_wire->post(in);
}

#if 0 // revisit
void EventLevel::disconnected()
{  
}
#endif
