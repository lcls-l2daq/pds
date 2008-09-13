#include "EventLevel.hh"
#include "EventStreams.hh"
#include "EventCallback.hh"
#include "pds/collection/Message.hh"
#include "pds/collection/Transition.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/StreamPortAssignment.hh"
#include "pds/utility/InletWire.hh"

#include "pds/service/NetServer.hh"
#include "pds/utility/BldServer.hh"
#include "pds/utility/NetDgServer.hh"

//#include "pds/collection/MessagePorts.hh"
//#include "McastDb.hh"
//#include "BcastRegistery.hh"

using namespace Pds;

static const unsigned MaxPayload = 2048;

static const unsigned ConnectTimeOut = 250; // 1/4 second

EventLevel::EventLevel(unsigned partition,
		       EventCallback& callback,
		       Arp* arp) :
  CollectionManager(Level::Event, partition,
		    MaxPayload, ConnectTimeOut, arp),
  _callback(callback),
  _streams(0)
{
}

EventLevel::~EventLevel() 
{
  if (_streams) delete _streams;
}

void EventLevel::attach()
{
  dotimeout(ConnectTimeOut);
  connect();
}

void EventLevel::message(const Node& hdr, 
			 const Message& msg) 
{
  InletWire* wire = _streams->wire(StreamParams::FrameWork);

  if (hdr.level() == Level::Control) {
    if (msg.type()==Message::Join) {
      arpadd(hdr);
      if (msg.size() > sizeof(Message)) {
	const StreamPortAssignment& a = static_cast<const StreamPortAssignment&>(msg);

	// Build the external servers (bld)
	unsigned i=0;
	for(StreamPortAssignmentIter iter(a); !iter.end(); ++iter, ++i) {
	  BldServer* srv = new BldServer(Src(i,0),
					 Ins( iter().portId() ),
					 EventStreams::netbufdepth);
	  printf("Assign fragment %d  %x/%d\n",
		 srv->id(),iter().address(),srv->server().portId());
	  wire->add_input(srv);
	  srv->server().join(iter(), Ins(header().ip()));
	}
	//  Make special note that these contributors are verbose;
	//  i.e. they supply data for more events than we are interested in.
	//
	//	_eb->verbose(1<<(i+1)-1);
	// Build the event data servers
	_mcast = a.client().address();
	Ins mcastIns(_mcast);
	while( _nextSegment-- ) {
	  NetDgServer* srv = new NetDgServer(Src(i, 0),
					     Ins( PdsStreamPorts::eventPort(i) ),
					     EventStreams::netbufdepth);
	  printf("Assign fragment %d  %x/%d\n",
		 srv->id(),_mcast,srv->server().portId());
	  wire->add_input(srv);
	  srv->server().join(mcastIns, Ins(header().ip()));

	  StreamPortAssignment s(Message::Join,Ins(_mcast,srv->server().portId()));
	  ucast(s,_segmentList[_nextSegment]);
	  i++;
	}
	_src = i;
      }
      else {
	ucast(msg, Ins(msg.reply_to()));
      }
    }
    else if (msg.type()==Message::Resign) {
      _dissolver = hdr;
      disconnect();
    }
    else if (msg.type()==Message::Transition) {
      const Transition& tr = static_cast<const Transition&>(msg);
      if (tr.id() != Transition::SoftL1)
	wire->post(tr);
    }
  }
  else if (hdr.level() == Level::Segment) {
    if (msg.type()==Message::Join) {
      if (!_mcast) {
	//  Keep a list of contributors until we know where to receive our data
	_segmentList[_nextSegment++] = msg.reply_to();
      }
      else {
	NetDgServer* srv = new NetDgServer(Src(_src, 0),
					   Ins( PdsStreamPorts::eventPort(_src) ),
					   EventStreams::netbufdepth);
	printf("Assign fragment %d  %x/%d\n",
	       srv->id(),_mcast,srv->server().portId());
	wire->add_input(srv);
	Ins mcastIns(_mcast);
	srv->server().join(mcastIns, Ins(header().ip()));

	StreamPortAssignment s(Message::Join,Ins(_mcast,srv->server().portId()));
	ucast(s,msg.reply_to());
	_src++;
      }
    }
  }
}

void EventLevel::connected(const Node& hdr, 
			   const Message& msg) 
{
  donottimeout();
  _mcast = 0;
  _nextSegment = 0;
  _src = 0;
  _streams = new EventStreams(*this);
  _streams->connect();
  _callback.attached(*_streams);
  Message join(Message::Join);
  mcast(join);
}

void EventLevel::timedout() 
{
  donottimeout();
  disconnect();
}

void EventLevel::disconnected()
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
