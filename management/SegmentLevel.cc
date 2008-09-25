#include "SegmentLevel.hh"
#include "SegStreams.hh"
#include "pds/collection/Node.hh"
#include "pds/collection/Message.hh"
#include "pds/collection/Transition.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/StreamPortAssignment.hh"
#include "pds/utility/InletWire.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/InletWireIns.hh"
#include "pds/utility/BldServer.hh"
#include "pds/utility/EvrServer.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/management/EventCallback.hh"
#include "pds/management/EbJoin.hh"
#include "pds/management/EbIStream.hh"

using namespace Pds;

static const unsigned MaxPayload = sizeof(StreamPortAssignment)+64*sizeof(Ins);
static const unsigned ConnectTimeOut = 250;
static const unsigned NetBufferDepth = 32;

SegmentLevel::SegmentLevel(unsigned partition,
			   int      index,
			   SegWireSettings& settings,
			   EventCallback& callback,
			   Arp* arp) :
  CollectionManager(Level::Segment, partition,
		    MaxPayload, ConnectTimeOut, arp),
  _settings   (settings),
  _index      (index),
  _callback   (callback),
  _streams    (0),
  _pool       (MaxPayload,32),
  _inlet      (0)
{
}

SegmentLevel::~SegmentLevel()
{
  if (_streams)  delete _streams;
  if (_inlet  )  delete _inlet;
}

void SegmentLevel::attach()
{
  dotimeout(ConnectTimeOut);
  connect();
}

void SegmentLevel::message(const Node& hdr, 
			   const Message& msg) 
{
  InletWire* bld_wire = _streams->wire(StreamParams::FrameWork);
  InletWire* pre_wire = _inlet ? _inlet->input() : bld_wire;

  if (msg.type() == Message::Join) {
    if (hdr.level() == Level::Control) {
      if (msg.size() > sizeof(Message)) {
	const StreamPortAssignment& a = static_cast<const StreamPortAssignment&>(msg);
	// Add BLD servers to the event builder on bld_wire
	for(StreamPortAssignmentIter iter(a); !iter.end(); ++iter) {
	  BldServer* srv = new BldServer(iter().ins,
					 iter().src,
					 NetBufferDepth);
	  bld_wire->add_input(srv);
	  srv->server().join(iter().ins, Ins(header().ip()));
	  printf("Assign fragment %d  %x/%d\n",
		 srv->id(),iter().ins.address(),srv->server().portId());
	}
      }
      else {
	arpadd(hdr);
	Message join(Message::Join,sizeof(Message));
	ucast(join,msg.reply_to());
      }
    }
    else if (hdr.level() == Level::Event) {
      if (msg.size() == sizeof(EbJoin)) {
	// Add vectored output clients on bld_wire
	const EbJoin& ebj = reinterpret_cast<const EbJoin&>(msg);
	int vector_id  = ebj.index();
	Ins ins = PdsStreamPorts::event(hdr.partition(),
					Level::Event,
					vector_id,
					_index);
	InletWireIns wireIns(vector_id, ins);
	bld_wire->add_output(wireIns);
	printf("SegmentLevel::message adding output %d to %x/%d\n",
	       vector_id, ins.address(), ins.portId());

	arpadd(hdr);
	EbJoin join(_index);
	ucast(join,msg.reply_to());
      }
      else {
	arpadd(hdr);
	Message join(Message::Join);
	ucast(join,msg.reply_to());
      }
    }
  }
  else if (msg.type()==Message::Resign && hdr.level()==Level::Control) {
    _dissolver = hdr;
    disconnect();
  }
  else if (msg.type()==Message::Transition && hdr.level()==Level::Control) {
    const Transition& tr = static_cast<const Transition&>(msg);
    if (tr.id() != Transition::L1Accept) {
      if (tr.phase() == Transition::Execute) {
	printf("SegmentLevel::message inserting transition\n");
	Transition* ntr = new(&_pool) Transition(tr);
	pre_wire->post(*ntr);
      }
      else {
	printf("SegmentLevel::message inserting datagram\n");
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

void SegmentLevel::connected(const Node& hdr, 
			     const Message& msg) 
{
  donottimeout();
  _streams = new SegStreams(*this,_index);
  _streams->connect();

  
  _callback.attached(*_streams);
  Message join(Message::Join);
  mcast(join);

  _inlet = new EbIStream(Src(Level::Segment, -1UL,
			     header().ip()),
			 header().ip(),
			 Level::Segment,
			 *_streams->wire(StreamParams::FrameWork));
  _inlet->input()->connect();
  _streams->wire(StreamParams::FrameWork)->add_input(_inlet->output());

  //  Build the EVR server
  Ins source(PdsStreamPorts::event(header().partition(),
				   Level::Segment));
  EvrServer* esrv = new EvrServer(source, 
				  Src(Level::Source,0,
				      hdr.ip()),
				  NetBufferDepth);
  _inlet->input()->add_input(esrv);
  esrv->server().join(source, Ins(header().ip()));
  printf("Assign evr %d  %x/%d\n",
	 esrv->id(),source.address(),source.portId());

  //  Add the L1 Data servers  
  _settings.connect(*_inlet->input(),
		    StreamParams::FrameWork, 
		    header().ip());
}

void SegmentLevel::timedout() 
{
  donottimeout();
  disconnect();
}

void SegmentLevel::disconnected() 
{
  if (_streams) {
    _streams->disconnect();
    _inlet->input()->disconnect();
    delete _streams;
    delete _inlet;
    _streams = 0;
    _inlet   = 0;
    _callback.dissolved(_dissolver);
  } else {
    _callback.failed(EventCallback::PlatformUnavailable);
  }
}
