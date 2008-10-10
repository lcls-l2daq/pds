#include "ControlLevel.hh"
#include "ControlStreams.hh"
#include "ControlCallback.hh"

#include "pds/collection/Message.hh"
#include "pds/utility/Transition.hh"
#include "pds/utility/InletWire.hh"
#include "pds/utility/NetDgServer.hh"
#include "pds/utility/StreamPorts.hh"

#include <time.h> // Required for timespec struct and nanosleep()

using namespace Pds;

ControlLevel::ControlLevel(unsigned platform,
			   ControlCallback& callback,
			   Arp* arp) :
  PartitionMember(platform, Level::Control, arp),
  _reason  (-1),
  _callback(callback),
  _streams (0),
  _reply   (Message::Ping)
{}

ControlLevel::~ControlLevel() 
{
  if (_streams) delete _streams;
}

bool ControlLevel::attach()
{
  start();
  if (connect()) {
    _streams = new ControlStreams(*this);
    _streams->connect();
    
    _callback.allocated(*_streams);
    
    Message join(Message::Join);
    mcast(join);
    return true;
  } else {
    _callback.failed(ControlCallback::PlatformUnavailable);
    return false;
  }
}

Message& ControlLevel::reply(Message::Type)
{
  return _reply;
}

void ControlLevel::allocated(const Allocate& alloc,
			     unsigned        index) 
{
  InletWire* wire = _streams->wire(StreamParams::FrameWork);

  // setup event servers
  unsigned nnodes     = alloc.nnodes();
  unsigned recorderid = 0;
  for (unsigned n=0; n<nnodes; n++) {
    const Node* node = alloc.node(n);
    if (node->level() == Level::Recorder) {
      // Add vectored output clients on bld_wire
      Ins ins = StreamPorts::event(header().platform(),
				   Level::Control,
				   index,
				   recorderid++);
      
      NetDgServer* srv = new NetDgServer(ins,
					 Src(*node),
					 ControlStreams::netbufdepth);
      wire->add_input(srv);
      Ins mcastIns(ins.address());
      srv->server().join(mcastIns, Ins(header().ip()));
      printf("ControlLevel::allocated assign fragment %d  %x/%d\n",
	     srv->id(),mcastIns.address(),srv->server().portId());
    }
  }
}

void ControlLevel::post     (const Transition& tr)
{
  InletWire* wire = _streams->wire(StreamParams::FrameWork);
  wire->post(tr);
}

void ControlLevel::post     (const InDatagram& in)
{
  InletWire* wire = _streams->wire(StreamParams::FrameWork);
  wire->post(in);
}

void ControlLevel::detach()
{
  if (_streams) {
    _streams->disconnect();
    delete _streams;
    _streams = 0;
    //    _callback.dissolved(_dissolver);
  } else {
    _callback.failed(ControlCallback::Reason(_reason));
  }
  cancel();
}
