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
			     unsigned nodes,
			     EventCallback& callback) :
  CollectionObserver(platform, partition),
  _nodes            (nodes),
  _callback         (callback),
  _streams          (0)
{
}

ObserverLevel::~ObserverLevel() 
{
  if (_streams) {
    for (int s = 0; s < StreamParams::NumberOfStreams; s++)
      delete _outlets[s];
  }
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

void ObserverLevel::allocated(const Allocation& alloc)
{
  InletWire* inlet = _streams->wire(StreamParams::FrameWork);

  // setup BLD and event servers
  unsigned partition  = alloc.partitionid();
  unsigned nnodes     = alloc.nnodes();
  unsigned segmentid  = 0;
  for (unsigned n=0; n<nnodes; n++) {
    const Node& node = *alloc.node(n);
    if (node.level() == Level::Segment) {
      Ins srvIns(StreamPorts::event(partition,
				    Level::Event,
				    0,  // this parameter is ignored for the port server assignment
				    segmentid).portId());
      NetDgServer* srv = new NetDgServer(srvIns,
					 node.procInfo(),
					 EventStreams::netbufdepth*EventStreams::MaxSize);
      inlet->add_input(srv);

      for(unsigned mask=_nodes,index=0; mask!=0; mask>>=1,index++) {
	if (mask&1) {
	  Ins mcastIns(StreamPorts::event(partition,
					  Level::Event,
					  index,
					  segmentid).address());
	  srv->server().join(mcastIns, Ins(header().ip()));
	  printf("ObserverLevel::allocated assign fragment %d  %x/%d\n",
		 srv->id(),mcastIns.address(),srv->server().portId());
	}
      }
      Ins bcastIns = StreamPorts::bcast(partition, Level::Event);
      srv->server().join(bcastIns, Ins(header().ip()));
      segmentid++;
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
