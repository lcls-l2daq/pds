#include "ControlLevel.hh"
#include "ControlStreams.hh"
#include "ControlCallback.hh"
#include "Query.hh"

#include "pds/collection/Message.hh"
#include "pds/collection/CollectionPorts.hh"
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
  _reason   (-1),
  _callback (callback),
  _streams  (0),
  _reply    (Message::Ping),
  _partitionid(-1)
{}

ControlLevel::~ControlLevel() 
{
  if (_streams) delete _streams;
}

unsigned ControlLevel::partitionid() const
{ return _partitionid; }

bool ControlLevel::attach()
{
  start();
  if (connect()) {
    _streams = new ControlStreams(*this);
    _streams->connect();
    
    _callback.allocated(*_streams);
    
    Message join(Message::Join);
    mcast(join);
    ucast(join, CollectionPorts::platform());

    return true;
  } else {
    _callback.failed(ControlCallback::PlatformUnavailable);
    return false;
  }
}

void ControlLevel::detach()
{
  if (_streams) {
    Message resign(Message::Resign);
    mcast(resign);
    ucast(resign, CollectionPorts::platform());

    _streams->disconnect();
    delete _streams;
    _streams = 0;
    //    _callback.dissolved(_dissolver);
  } else {
    _callback.failed(ControlCallback::Reason(_reason));
  }
  cancel();
}

Message& ControlLevel::reply(Message::Type)
{
  return _reply;
}

void ControlLevel::allocated(const Allocation& alloc,
			     unsigned          index) 
{
  _allocation = alloc;
  PartitionAllocation pa(_allocation);
  ucast( pa, CollectionPorts::platform());

  InletWire* wire = _streams->wire(StreamParams::FrameWork);

  // setup event servers
  unsigned nnodes     = alloc.nnodes();
  unsigned recorderid = 0;
  for (unsigned n=0; n<nnodes; n++) {
    const Node* node = alloc.node(n);
    if (node->level() == Level::Recorder) {
      // Add vectored output clients on bld_wire
      Ins ins = StreamPorts::event(alloc.partitionid(),
				   Level::Control,
				   index,
				   recorderid++);
      
      NetDgServer* srv = new NetDgServer(ins,
					 node->procInfo(),
					 ControlStreams::netbufdepth);
      wire->add_input(srv);
      Ins mcastIns(ins.address());
      srv->server().join(mcastIns, Ins(header().ip()));
    }
  }
}

void ControlLevel::dissolved()
{
  Allocation alloc(_allocation.partition(),
		   _allocation.dbpath(),
		   _allocation.partitionid());
  _allocation = alloc;
  PartitionAllocation pa(alloc);
  ucast( pa, CollectionPorts::platform());

  _streams->disconnect();
  _streams->connect();
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

void ControlLevel::message(const Node& hdr, const Message& msg)
{
  if (msg.type () == Message::Query &&
      hdr.level() == Level::Source) {
    const Query& query = reinterpret_cast<const Query&>(msg);
    if (query.type() == Query::Group) {
      _partitionid = 
	reinterpret_cast<const PartitionGroup&>(query).partitionid();
    }
    else if (query.type() == Query::Partition) {
      PartitionAllocation pa(_allocation);
      ucast( pa, msg.reply_to() );
    }
  }
  PartitionMember::message(hdr,msg);
}
