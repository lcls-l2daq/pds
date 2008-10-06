#include "EventLevel.hh"
#include "EventStreams.hh"
#include "EventCallback.hh"
#include "pds/collection/Message.hh"
#include "pds/utility/Transition.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/StreamPortAssignment.hh"
#include "pds/utility/InletWire.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/InletWireIns.hh"

#include "pds/service/NetServer.hh"
#include "pds/utility/BldServer.hh"
#include "pds/utility/NetDgServer.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/management/EbIStream.hh"
#include "pds/management/EbJoin.hh"


using namespace Pds;

static const unsigned MaxPayload = sizeof(StreamPortAssignment)+64*sizeof(Ins);

static const unsigned ConnectTimeOut = 250; // 1/4 second

EventLevel::EventLevel(unsigned platform,
		       unsigned id,
		       EventCallback& callback,
		       Arp* arp) :
  CollectionManager(Level::Event, platform,
		    MaxPayload, ConnectTimeOut, arp),
  _index      (id),
  _callback   (callback),
  _streams    (0),
  _pool       (sizeof(CDatagram), 32),
  _inlet      (0)
{
}

EventLevel::~EventLevel() 
{
  if (_streams) delete _streams;
  if (_inlet  ) delete _inlet;
}

void EventLevel::attach()
{
  if (connect()) {
    _streams = new EventStreams(*this,_index);
    _streams->connect();

    _inlet = new EbIStream(Src(Level::Event,_index,
                               header().ip()),
                           header().ip(),
                           Level::Event,
                           *_streams->wire(StreamParams::FrameWork));
    _inlet->input()->connect();
    _streams->wire(StreamParams::FrameWork)->add_input(_inlet->output());

    _callback.attached(*_streams);
    //    _rivals.insert(_index, msg.reply_to()); // revisit
    Message join(Message::Join);
    mcast(join);
  } else {
    _callback.failed(EventCallback::PlatformUnavailable);
  }
}

void EventLevel::message(const Node& hdr, 
			 const Message& msg) 
{
  InletWire* bld_wire = _streams->wire(StreamParams::FrameWork);
  InletWire* pre_wire = _inlet ? _inlet->input() : bld_wire;

  if (msg.type()==Message::Join) {
    if (hdr.level() == Level::Control) {
      if (msg.size() > sizeof(Message)) {
	const StreamPortAssignment& a = static_cast<const StreamPortAssignment&>(msg);

	// Add BLD servers to the event builder on bld_wire
	for(StreamPortAssignmentIter iter(a); !iter.end(); ++iter) {
	  printf("Assign bld %08x  %x/%d\n",
		 iter().src.did(),
		 iter().ins.address(),iter().ins.portId());
	  BldServer* srv = new BldServer(iter().ins,
					 iter().src,
					 EventStreams::netbufdepth);
	  bld_wire->add_input(srv);
	  srv->server().join(iter().ins, Ins(header().ip()));
	}
      }
      else {
	arpadd(hdr);
	Message join(Message::Join,sizeof(Message));
	ucast(join, msg.reply_to());
      }
    }
    else if (hdr.level() == Level::Segment) {
      if (msg.size() == sizeof(EbJoin)) {
	// Add Datagram server to the event builder on pre_wire
	const EbJoin& ebj = reinterpret_cast<const EbJoin&>(msg);
	Ins streamPort(StreamPorts::event(header().platform(),
					     Level::Event,
					     _index,
					     ebj.index()));
	printf("Joining segment from %08x to port %x/%d\n",
	       hdr.ip(),streamPort.address(),streamPort.portId());
	//	NetDgServer* srv = new NetDgServer(Ins(streamPort.portId()),
	NetDgServer* srv = new NetDgServer(streamPort,
					   Src(Level::Segment,ebj.index(),
					       hdr.ip()),
					   EventStreams::netbufdepth);
	pre_wire->add_input(srv);
	Ins mcastIns(streamPort.address());
	srv->server().join(mcastIns, Ins(header().ip()));
	printf("Assign fragment %d  %x/%d\n",
	       srv->id(),mcastIns.address(),srv->server().portId());
      }
      else {
	arpadd(hdr);
	EbJoin ebj(_index);
	ucast(ebj,msg.reply_to());
      }
    }
    else if (hdr.level() == Level::Event) {
      // Transition datagrams
      if (msg.size() != sizeof(EbJoin) ||
	  !_rivals.insert(reinterpret_cast<const EbJoin&>(msg).index(),
			  msg.reply_to())) {
	arpadd(hdr);
	EbJoin ebj(_index);
	ucast(ebj,msg.reply_to());
      }
    }
    else if (hdr.level() == Level::Recorder) {
      if (msg.size() == sizeof(EbJoin)) {
	const EbJoin& ebj = reinterpret_cast<const EbJoin&>(msg);
	int vector_id  = ebj.index();
	const Ins& ins = StreamPorts::event(hdr.platform(),
					       Level::Recorder,
					       vector_id,
					       _index);
	InletWireIns wireIns(vector_id, ins);
	bld_wire->add_output(wireIns);
	printf("EventLevel::message adding output %d to %x/%d\n",
	       vector_id, ins.address(), ins.portId());

	arpadd(hdr);
	EbJoin join(_index);
	ucast(join, msg.reply_to());
      }
      else {
	arpadd(hdr);
	Message join(Message::Join,sizeof(Message));
	ucast(join, msg.reply_to());
      }
    }
  }
  else if (msg.type()==Message::Transition && hdr.level()==Level::Control) {
    const Transition& tr = static_cast<const Transition&>(msg);
    if (tr.id() != Transition::L1Accept) {
      OutletWireIns* dst;
      if (tr.phase() == Transition::Execute) {
	printf("EventLevel::message inserting transition\n");
	Transition* ntr = new(&_pool) Transition(tr);
	pre_wire->post(*ntr);
      }
      else if ((dst=_rivals.lookup(tr.sequence())) &&
	       dst->id() != (unsigned)_index) {
	printf("EventLevel::message inserting datagram\n");
	CDatagram* ndg = 
	  new(&_pool) CDatagram(Datagram(tr,
					 TC(TypeNumPrimary::Id_XTC),
					 Src(Level::Segment,_index,
					     header().ip())));
	pre_wire->post(*ndg);
      }
    }
  }
}

#if 0 // revisit
void EventLevel::disconnected()
{  
  if (_streams) {
    _streams->disconnect();
    _inlet->input()->disconnect();
    delete _streams;
    delete _inlet;
    _streams = 0;
    _inlet   = 0;
    _callback.dissolved(_dissolver);
  }
}
#endif
