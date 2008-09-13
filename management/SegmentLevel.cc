#include "SegmentLevel.hh"
#include "SegStreams.hh"
#include "pds/collection/Node.hh"
#include "pds/collection/Message.hh"
#include "pds/collection/Transition.hh"
#include "pds/utility/StreamPortAssignment.hh"
#include "pds/utility/InletWire.hh"
#include "pds/utility/InletWireIns.hh"
#include "pds/management/EventCallback.hh"

using namespace Pds;

static const unsigned MaxPayload = sizeof(StreamPortAssignment)+64*sizeof(Ins);
static const unsigned ConnectTimeOut = 250;
static TC SoftL1Tc(TypeIdQualified(TypeNumPrimary::Id_ModuleContainer),
		   TypeIdQualified(TypeNumPrimary::Id_XTC));

SegmentLevel::SegmentLevel(unsigned partition,
			   const SegmentOptions& options,
			   SegWireSettings& settings,
			   EventCallback& callback,
			   Arp* arp) :
  CollectionManager(Level::Segment, partition,
		    MaxPayload, ConnectTimeOut, arp),
  _options   (options),
  _settings  (settings),
  _callback  (callback),
  _streams   (0),
  _transition(new char[sizeof(Datagram)])
{
}

SegmentLevel::~SegmentLevel()
{
  delete[] _transition;
  if (_streams)  delete _streams;
}

void SegmentLevel::attach()
{
  dotimeout(ConnectTimeOut);
  connect();
}

void SegmentLevel::message(const Node& hdr, 
			   const Message& msg) 
{
  InletWire* wire = _streams->wire(StreamParams::FrameWork);

  if (hdr.level() == Level::Control) {
    switch (msg.type()) {
    case Message::Join:
      arpadd(hdr);
      ucast(msg,Ins(msg.reply_to()));
      break;
    case Message::Resign:
      _dissolver = hdr;
      //      while(_dst)
      //	_streams->remove_output(--_dst);
      disconnect();
      break;
    case Message::Transition:
      {
        const Transition& tr = static_cast<const Transition&>(msg);
	if (tr.id() != Transition::SoftL1)
	  wire->post(tr);
	else {
	  Datagram& dg = *new(_transition) 
	    Datagram(Sequence(Sequence::Event,
			      Sequence::L1Accept,
			      0, reinterpret_cast<const L1Transition*>(&tr)->key()),
		     SoftL1Tc,
		     0,
		     Src((_options.crate<<8) | _options.slot,
			 (_options.detector<<8) | _options.module));
	  printf("SegmentLevel::message transition seq %08x/%08x\n",
		 dg.highAll(), dg.low());
	  wire->post(dg);
	}
      }
      break;
    }
  }
  else if (hdr.level() == Level::Event) {
    switch (msg.type()) {
    case Message::Join:
      if (msg.size() != sizeof(Message)) {
	const StreamPortAssignment& a = *reinterpret_cast<const StreamPortAssignment*>(&msg);
	const Ins& ins = a.client();
	InletWireIns wireIns(_dst, ins, ins.address());
	wire->add_output(wireIns);
	printf("SegmentLevel::message adding output %d to %x/%d\n",
	       _dst, ins.address(), ins.portId());
	_dst++;
      }
      else {
	arpadd(hdr);
	ucast(msg,Ins(msg.reply_to()));
      }
    case Message::Resign:
    case Message::Transition:
      break;
    }
  }
}

void SegmentLevel::connected(const Node& hdr, 
			     const Message& msg) 
{
  donottimeout();
  _dst = 0;
  _streams = new SegStreams(_options, _settings, *this);
  _streams->connect();
  _callback.attached(*_streams);
  Message join(Message::Join);
  mcast(join);
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
    delete _streams;
    _streams = 0;
    _callback.dissolved(_dissolver);
  } else {
    _callback.failed(EventCallback::PlatformUnavailable);
  }
}
