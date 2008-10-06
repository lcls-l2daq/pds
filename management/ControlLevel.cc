#include "ControlLevel.hh"
#include "ControlStreams.hh"
#include "ControlCallback.hh"

#include "pds/collection/Message.hh"
#include "pds/utility/Transition.hh"
#include "pds/utility/InletWire.hh"
#include "pds/utility/NetDgServer.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/StreamPortAssignment.hh"
#include "EbJoin.hh"

#include <time.h> // Required for timespec struct and nanosleep()

using namespace Pds;

static const unsigned ControlId = 0;
static const unsigned MaxPayload = sizeof(Transition);
static const unsigned ConnectTimeOut = 250; // 1/4 second

ControlLevel::ControlLevel(unsigned platform,
			   ControlCallback& callback,
			   Arp* arp) :
  CollectionManager(Level::Control, platform,
		    MaxPayload, ConnectTimeOut, arp),
  _reason(-1),
  _callback(callback),
  _streams(0),
  _buffer(new char[sizeof(StreamPortAssignment)+32*sizeof(StreamPort)]),
  _bldServers(new(_buffer)StreamPortAssignment(Message::Join))
{}

ControlLevel::~ControlLevel() 
{
  if (_streams) delete _streams;
  delete[] _buffer;
}

void ControlLevel::attach()
{
  if (connect()) {
    _streams = new ControlStreams(*this);
    _streams->connect();
    
    _callback.allocated(*_streams);
    
    Message join(Message::Join);
    mcast(join);
  } else {
    _callback.failed(ControlCallback::PlatformUnavailable);
  }
}

void ControlLevel::add_bld(int id)
{
  _bldServers->add_server(StreamPort(StreamPorts::bld(id),
				     Src(Level::Segment,id,
					 BLD,id)));
}

#if 0
void ControlLevel::check_complete(const Node& hdr, bool isjoining)
{
  if (_partition) {
    switch (_partition->add(hdr)) {
    case Partition::Ignored:
      break;
    case Partition::New:
      if (_partition->iscomplete()) {
	donottimeout();
	{
	  //  Add a 1/4 second delay to avoid a race condition in the
	  //  collection mechanism.  The fragment levels need to finish
	  //  acquiring a list of event nodes before they're really ready.
	  struct timespec delay;
	  delay.tv_nsec = 250000000; // 1/4 second
	  delay.tv_sec = 0;
	  nanosleep(&delay, 0);
	}
	_callback.ready(_partition);
      }
      break;
    case Partition::AlreadyThere:
      if (isjoining) {
	PartitionException excp(hdr, PartitionException::LateJoin);
	_callback.notready(_partition, &excp);
      }
      break;
    case Partition::NotExpected:
      {
	PartitionException excp(hdr, PartitionException::NotExpected);
	_callback.notready(_partition, &excp);
      }
      break;
    }
  }
}
#endif

void ControlLevel::message(const Node& hdr, 
			   const Message& msg) 
{
  if (msg.type()==Message::Join) {
    if (hdr.level() == Level::Segment ||
	hdr.level() == Level::Event) {
      arpadd(hdr);
      ucast(*_bldServers,msg.reply_to());
    }
    else if (hdr.level() == Level::Recorder) {
      if (msg.size() == sizeof(EbJoin)) {
	// Build the event data servers
	const EbJoin& ebj = reinterpret_cast<const EbJoin&>(msg);
	int vector_id  = ebj.index();
	Ins streamPort(StreamPorts::event(header().platform(),
                                          Level::Control,
                                          0,
                                          vector_id));
	NetDgServer* srv = new NetDgServer(streamPort,
					   Src(Level::Recorder,vector_id,
					       hdr.ip()),
					   ControlStreams::netbufdepth);
	_streams->wire(StreamParams::FrameWork)->add_input(srv);
	Ins mcastIns(streamPort.address());
	srv->server().join(mcastIns, Ins(header().ip()));
	printf("Assign fragment %d  %x/%d\n",
	       srv->id(),mcastIns.address(),srv->server().portId());
      }
      else {
	arpadd(hdr);
	EbJoin ebj(0);
	ucast(ebj,msg.reply_to());
      }
    }
    //    check_complete(hdr);
  }
  if (msg.type()==Message::Transition) {
    const Transition& tr = reinterpret_cast<const Transition&>(msg);
    printf("Transition %d:%d from Level %d pid %d uid %d ip %08x\n",
	   tr.id(), tr.phase(),
	   hdr.level(), hdr.pid(), hdr.uid(), hdr.ip());
  }
}

#if 0 // revisit
void ControlLevel::disconnected()
{
  if (_streams) {
    _streams->disconnect();
    delete _streams;
    _streams = 0;
    _callback.dissolved(_dissolver);
  } else {
    _callback.failed(ControlCallback::Reason(_reason));
  }
}
#endif
