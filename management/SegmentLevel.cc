#include "SegmentLevel.hh"
#include "SegStreams.hh"
#include "pds/collection/Node.hh"
#include "pds/collection/Message.hh"
#include "pds/utility/Transition.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/InletWire.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/InletWireIns.hh"
#include "pds/utility/BldServer.hh"
#include "pds/utility/EvrServer.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/management/EventCallback.hh"
#include "pds/management/EbIStream.hh"

using namespace Pds;

static const unsigned NetBufferDepth = 32;

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
  return Src(n);
}

SegmentLevel::SegmentLevel(unsigned platform,
			   SegWireSettings& settings,
			   EventCallback& callback,
			   Arp* arp) :
  PartitionMember(platform, Level::Segment, arp),
  _settings      (settings),
  _callback      (callback),
  _streams       (0),
  _inlet         (0),
  _reply         (Message::Ping)
{
}

SegmentLevel::~SegmentLevel()
{
  if (_streams)  delete _streams;
  if (_inlet  )  delete _inlet;
}

bool SegmentLevel::attach()
{
  start();
  if (connect()) {
    _streams = new SegStreams(*this);
    _streams->connect();

    _callback.attached(*_streams);

    _inlet = new EbIStream(Src(header()),
                           header().ip(),
                           Level::Segment,
                           *_streams->wire(StreamParams::FrameWork));
    _inlet->input()->connect();
    _streams->wire(StreamParams::FrameWork)->add_input(_inlet->output());

    //  Build the EVR server
    Ins source(StreamPorts::event(header().platform(),
                                  Level::Segment));
    EvrServer* esrv = new EvrServer(source, 
                                    Src(header()),
                                    NetBufferDepth); // revisit
    _inlet->input()->add_input(esrv);
    esrv->server().join(source, Ins(header().ip()));
    printf("Assign evr %d  %x/%d\n",
           esrv->id(),source.address(),source.portId());

    //  Add the L1 Data servers  
    _settings.connect(*_inlet->input(),
                      StreamParams::FrameWork, 
                      header().ip());

    Message join(Message::Ping);
    mcast(join);
    return true;
  } else {
    _callback.failed(EventCallback::PlatformUnavailable);
    return false;
  }
}

Message& SegmentLevel::reply    (Message::Type type)
{
  //  Need to append L1 detector info (Src) to the reply
  return _reply;
}

void    SegmentLevel::allocated(const Allocate& alloc,
				unsigned        index) 
{
  InletWire* bld_wire = _streams->wire(StreamParams::FrameWork);

  // setup BLD and event servers
  unsigned partition= alloc.partitionid();
  unsigned nnodes   = alloc.nnodes();
  unsigned vectorid = 0;
  for (unsigned n=0; n<nnodes; n++) {
    const Node& node = *alloc.node(n);
    if (_is_bld(node)) {
      Ins ins( _bld_ins(node) );
      BldServer* srv = new BldServer(ins, _bld_src(node), NetBufferDepth);
      bld_wire->add_input(srv);
      srv->server().join(ins, Ins(header().ip()));
      printf("SegmentLevel::allocated assign bld  fragment %d  %x/%d\n",
	     srv->id(),ins.address(),srv->server().portId());
    }
    else if (node.level()==Level::Event) {
      // Add vectored output clients on bld_wire
      Ins ins = StreamPorts::event(partition,
				   Level::Event,
				   vectorid,
				   index);
      InletWireIns wireIns(vectorid, ins);
      bld_wire->add_output(wireIns);
      printf("SegmentLevel::allocated adding output %d to %x/%d\n",
	     vectorid, ins.address(), ins.portId());
      vectorid++;
    }
  }
}

void    SegmentLevel::post     (const Transition& tr)
{
  InletWire* bld_wire = _streams->wire(StreamParams::FrameWork);
  InletWire* pre_wire = _inlet ? _inlet->input() : bld_wire;
  pre_wire->post(tr);
}

void    SegmentLevel::post     (const InDatagram& in)
{
  InletWire* bld_wire = _streams->wire(StreamParams::FrameWork);
  InletWire* pre_wire = _inlet ? _inlet->input() : bld_wire;
  pre_wire->post(in);
}

void SegmentLevel::detach() 
{
  if (_streams) {
    _streams->disconnect();
    _inlet->input()->disconnect();
    delete _streams;
    delete _inlet;
    _streams = 0;
    _inlet   = 0;
    _callback.dissolved(header());
  }
  cancel();
}
