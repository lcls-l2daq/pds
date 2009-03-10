#include "ObserverLevel.hh"
#include "EventStreams.hh"
#include "EventCallback.hh"
#include "pds/collection/Message.hh"
#include "pds/utility/Transition.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/InletWire.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/InletWireIns.hh"
#include "pds/utility/OutletWire.hh"

#include "pds/service/NetServer.hh"
#include "pds/utility/BldServer.hh"
#include "pds/utility/NetDgServer.hh"
#include "pds/management/EbIStream.hh"

namespace Pds {
  class OpenOutlet : public OutletWire {
  public:
    OpenOutlet(Outlet& outlet) : OutletWire(outlet) {}
    virtual Transition* forward(Transition* dg) { return 0; }
    virtual InDatagram* forward(InDatagram* dg) { return 0; }
    virtual void bind(unsigned id, const Ins& node) {}
    virtual void unbind(unsigned id) {}
  };
};

using namespace Pds;

static inline bool _is_bld(const Node& n)
{
  return n.level() == Level::Reporter;
}

static inline Ins _bld_ins(const Node& n)
{
  return Ins(n.ip(), StreamPorts::bld(0).portId());
}

static inline Src _bld_src(const Node& n)
{
  return n.procInfo();
}

static const unsigned MaxPayload = 2048;

static const unsigned ConnectTimeOut = 250; // 1/4 second

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
  if (!connect()) {
    _callback.failed(EventCallback::PlatformUnavailable);
    return false;
  }

  _streams = new EventStreams(*this);
   for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
     delete _streams->stream(s)->outlet()->wire();
     _outlets[s] = new OpenOutlet(*_streams->stream(s)->outlet());
   }
  _streams->connect();
  
  _inlet = new EbIStream(header().procInfo(),
			 header().ip(),
			 Level::Event,
			 *_streams->wire(StreamParams::FrameWork));
  _inlet->input()->connect();
  _streams->wire(StreamParams::FrameWork)->add_input(_inlet->output());
  
  _callback.attached(*_streams);
  return true;
}

void ObserverLevel::allocated(const Allocate& alloc,
			      unsigned index)
{
  InletWire* bld_wire = _streams->wire(StreamParams::FrameWork);
  InletWire* pre_wire = _inlet ? _inlet->input() : bld_wire;

  // setup BLD and event servers
  unsigned partition  = alloc.partitionid();
  unsigned nnodes     = alloc.nnodes();
  unsigned segmentid  = 0;
  for (unsigned n=0; n<nnodes; n++) {
    const Node& node = *alloc.node(n);
    if (_is_bld(node)) {
      Ins ins( _bld_ins(node) );
      BldServer* srv = new BldServer(ins, _bld_src(node), EventStreams::netbufdepth);
      bld_wire->add_input(srv);
      srv->server().join(ins, Ins(header().ip()));
      printf("ObserverLevel::allocated assign bld  fragment %d  %x/%d\n",
	     srv->id(),ins.address(),srv->server().portId());
    }
    else if (node.level() == Level::Segment) {
      // Add vectored output clients on bld_wire
      Ins ins = StreamPorts::event(partition,
				   Level::Event,
				   index,
				   segmentid++);
      
      NetDgServer* srv = new NetDgServer(ins,
					 node.procInfo(),
					 EventStreams::netbufdepth*EventStreams::MaxSize);
      pre_wire->add_input(srv);
      Ins mcastIns(ins.address());
      srv->server().join(mcastIns, Ins(header().ip()));
      printf("EventLevel::allocated assign fragment %d  %x/%d\n",
	     srv->id(),mcastIns.address(),srv->server().portId());
    }
  }
}

void ObserverLevel::post     (const Transition& tr)
{
  InletWire* bld_wire = _streams->wire(StreamParams::FrameWork);
  InletWire* pre_wire = _inlet ? _inlet->input() : bld_wire;
  pre_wire->post(tr);
}

void ObserverLevel::dissolved()
{
  // detach the ToEb
  InletWireServer* wire = 
    dynamic_cast<InletWireServer*>(_streams->wire(StreamParams::FrameWork));
  wire->unmanage(wire->server(0));

  // destroy all inputs and outputs
  _inlet->input()->disconnect();
  _streams->disconnect();

  // reattach the ToEb
  _streams->connect();
  _inlet->input()->connect();
  wire->add_input(_inlet->output());
}

void ObserverLevel::detach()
{
  if (_streams) {
    _streams->disconnect();
    _inlet->input()->disconnect();
    delete _streams;
    delete _inlet;
    _streams = 0;
    _inlet   = 0;
    //    _callback.dissolved(_dissolver);
  }
  cancel();
}
