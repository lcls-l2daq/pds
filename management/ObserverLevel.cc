#include "ObserverLevel.hh"
#include "ObserverStreams.hh"
#include "EventStreams.hh"
#include "EventCallback.hh"
#include "pds/collection/Message.hh"
#include "pds/utility/Transition.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/InletWire.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/InletWireIns.hh"
#include "pds/utility/OpenOutlet.hh"

#include "pds/utility/NetDgServer.hh"

using namespace Pds;

ObserverLevel::ObserverLevel(unsigned platform,
			     const char* partition,
			     unsigned node,
			     EventCallback& callback) :
  CollectionObserver(platform, partition, node),
  _callback         (callback),
  _streams          (0)
{
}

ObserverLevel::~ObserverLevel() 
{
  for (int s = 0; s < StreamParams::NumberOfStreams; s++)
    delete _outlets[s];
}

bool ObserverLevel::attach()
{
  start();
  if (connect()) {
    _streams = new ObserverStreams(*this);
    for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
      delete _streams->stream(s)->outlet()->wire();
      _outlets[s] = new OpenOutlet(*_streams->stream(s)->outlet());
    }
    _streams->connect();
  
    _callback.attached(*_streams);
    
    return true;
  }
  else {
    _callback.failed(EventCallback::PlatformUnavailable);
    return false;
  }
}

void ObserverLevel::allocated(const Allocation& alloc,
			      unsigned index)
{
  InletWire* inlet = _streams->wire(StreamParams::FrameWork);

  // setup BLD and event servers
  unsigned partition  = alloc.partitionid();
  unsigned nnodes     = alloc.nnodes();
  unsigned segmentid  = 0;
  for (unsigned n=0; n<nnodes; n++) {
    const Node& node = *alloc.node(n);
    if (node.level() == Level::Segment) {
      // Add vectored output clients on bld_wire
      Ins ins = StreamPorts::event(partition,
				   Level::Event,
				   index,
				   segmentid++);
      
      Ins srvIns(ins.portId());
      NetDgServer* srv = new NetDgServer(srvIns,
					 node.procInfo(),
					 EventStreams::netbufdepth*EventStreams::MaxSize);
      inlet->add_input(srv);
      Ins mcastIns(ins.address());
      srv->server().join(mcastIns, Ins(header().ip()));
      Ins bcastIns = StreamPorts::bcast(partition, Level::Event);
      srv->server().join(bcastIns, Ins(header().ip()));
      printf("EventLevel::allocated assign fragment %d  %x/%d\n",
	     srv->id(),mcastIns.address(),srv->server().portId());
    }
  }
}

void ObserverLevel::post     (const Transition& tr)
{
  _streams->wire(StreamParams::FrameWork)->post(tr);
}

void ObserverLevel::post     (const InDatagram& in)
{
  _streams->wire(StreamParams::FrameWork)->post(in);
}

void ObserverLevel::dissolved()
{
  _streams->disconnect();
  _streams->connect();
}

void ObserverLevel::detach()
{
  if (_streams) {
    _streams->disconnect();
    delete _streams;
    _streams = 0;
    //    _callback.dissolved(_dissolver);
  }
  cancel();
}
