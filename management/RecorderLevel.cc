#include "RecorderLevel.hh"
#include "EventStreams.hh"
#include "EventCallback.hh"
#include "pds/collection/Message.hh"
#include "pds/collection/Transition.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/StreamPortAssignment.hh"
#include "pds/utility/InletWire.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/InletWireIns.hh"

#include "pds/service/NetServer.hh"
#include "pds/utility/BldServer.hh"
#include "pds/utility/NetDgServer.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/management/EbJoin.hh"

using namespace Pds;

static const unsigned MaxPayload = 2048;

static const unsigned ConnectTimeOut = 250; // 1/4 second

RecorderLevel::RecorderLevel(unsigned partition,
			     unsigned id,
			     EventCallback& callback,
			     Arp* arp) :
  CollectionManager(Level::Recorder, partition,
		    MaxPayload, ConnectTimeOut, arp),
  _index   (id),
  _callback(callback),
  _streams (0),
  _pool    (sizeof(CDatagram), 32)
{
}

RecorderLevel::~RecorderLevel() 
{
  if (_streams) delete _streams;
}

void RecorderLevel::attach()
{
  dotimeout(ConnectTimeOut);
  connect();
}

void RecorderLevel::message(const Node& hdr, 
			    const Message& msg) 
{
  InletWire* wire = _streams->wire(StreamParams::FrameWork);

  if (msg.type()==Message::Join) {
    if (hdr.level() == Level::Control) {
      if (msg.size() == sizeof(EbJoin)) {
	const EbJoin& ebj = reinterpret_cast<const EbJoin&>(msg);
	int vector_id  = ebj.index();
	const Ins& ins = PdsStreamPorts::event(hdr.partition(),
					       Level::Control,
					       vector_id,
					       _index);
	InletWireIns wireIns(vector_id, ins);
	wire->add_output(wireIns);
	printf("RecorderLevel::message adding output %d to %x/%d\n",
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
    else if (hdr.level() == Level::Event) {
      if (msg.size() == sizeof(EbJoin)) {
	// Build the event data servers
	const EbJoin& ebj = reinterpret_cast<const EbJoin&>(msg);
	int vector_id  = ebj.index();
	Ins streamPort(PdsStreamPorts::event(header().partition(),
					     Level::Recorder,
					     _index,
					     vector_id));
	//      NetDgServer* srv = new NetDgServer(Ins(streamPort.portId()),
	NetDgServer* srv = new NetDgServer(streamPort,
					   Src(Level::Event,vector_id,
					       hdr.ip()),
					   EventStreams::netbufdepth);
	wire->add_input(srv);
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
    else if (hdr.level() == Level::Recorder) {
      // Transition datagrams
      if (msg.size() != sizeof(EbJoin) ||
	  !_rivals.insert(reinterpret_cast<const EbJoin&>(msg).index(),
			  msg.reply_to())) {
	arpadd(hdr);
	EbJoin ebj(_index);
	ucast(ebj,msg.reply_to());
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
      OutletWireIns* dst;
      if (tr.phase() == Transition::Execute) {
	Transition* ntr = new(&_pool) Transition(tr);
	wire->post(*ntr);
      }
      else if ((dst=_rivals.lookup(tr.sequence())) &&
	       dst->id() != (unsigned)_index) {
	CDatagram* ndg =
	  new(&_pool) CDatagram(Datagram(tr, 
					 TC(TypeNumPrimary::Id_XTC),
					 Src(Level::Segment,_index,
					     header().ip())));
	wire->post(*ndg);
      }
    }
  }
}

void RecorderLevel::connected(const Node& hdr, 
			      const Message& msg) 
{
  donottimeout();
  _streams = new EventStreams(*this,_index);
  _streams->connect();

  _callback.attached(*_streams);

  _rivals.insert(_index, msg.reply_to());
  Message join(Message::Join);
  mcast(join);
}

void RecorderLevel::timedout() 
{
  _dissolver = header();
  donottimeout();
  disconnect();
}

void RecorderLevel::disconnected()
{
  
  if (_streams) {
    _streams->disconnect();
    delete _streams;
    _streams = 0;
    _callback.dissolved(_dissolver);
  } else {
    _callback.failed(EventCallback::PlatformUnavailable);
  }
}
