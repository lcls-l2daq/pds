#include "SegmentLevel.hh"
#include "SegStreams.hh"
#include "pds/collection/Node.hh"
#include "pds/collection/Message.hh"
#include "pds/utility/Transition.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/OutletWire.hh"
#include "pds/utility/InletWire.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/InletWireIns.hh"
#include "pds/utility/EvrServer.hh"
#include "pds/utility/ToEventWireScheduler.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/management/EventBuilder.hh"
#include "pds/management/EventCallback.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <unistd.h>

using namespace Pds;

//#ifdef BUILD_LARGE_STREAM_BUFFER
//static const unsigned NetBufferDepth = 1024;
//#else
//static const unsigned NetBufferDepth = 32;
//#endif

static const unsigned NetBufferDepth = 1024;

SegmentLevel::SegmentLevel(unsigned platform,
         SegWireSettings& settings,
         EventCallback& callback,
         Arp* arp,
         int slowEb
         ) :
  PartitionMember(platform, Level::Segment, slowEb, arp),
  _settings      (settings),
  _callback      (callback),
  _streams       (0),
  _evr           (0),
  _reply         (settings.sources())
{
  if (settings.pAliases()) {
    _aliasReply = *(settings.pAliases());
  } else {
    // create empty alias reply
    new (&_aliasReply) AliasReply();
  }
}

SegmentLevel::~SegmentLevel()
{
  if (_streams)  delete _streams;
}

bool SegmentLevel::attach()
{
  start();
  while(1) {
    if (connect()) {
      _streams = new SegStreams(*this,
                                _settings.max_event_size (),
                                _settings.max_event_depth(),
                                _settings.pAliases() && !_settings.pAliases()->empty() ?
                                _settings.pAliases()->front().aliasName() : 0);
      _streams->connect();

      _callback.attached(*_streams);

      //  Add the L1 Data servers
      _settings.connect(*_streams->wire(StreamParams::FrameWork),
                        StreamParams::FrameWork,
                        header().ip());

      _reply.ready(true);
      mcast(_reply);
      return true;
    } else {
      _callback.failed(EventCallback::PlatformUnavailable);
      sleep(1);
    }
  }
  return false;
}

Message& SegmentLevel::reply    (Message::Type type)
{
  switch (type) {
    case Message::Alias:
      //  Need to append L1 detector alias (SrcAlias) to the reply
      return _aliasReply;

    case Message::Ping:
    default:
      //  Need to append L1 detector info (Src) to the reply
      return _reply;
  }
}

void    SegmentLevel::allocated(const Allocation& alloc,
        unsigned          index)
{
  InletWire& inlet = *_streams->wire(StreamParams::FrameWork);

  //!!! segment group support
  for (unsigned n=0; n<alloc.nnodes(); n++) {
    const Node& node = *alloc.node(n);
    if (node.level() == Level::Segment &&
	node == _header) {
      if (node.group() != _header.group())
	{
	  _header.setGroup(node.group());
	  printf("Assign group to %d\n", _header.group());
	}

      _contains = node.transient()?_transientXtcType:_xtcType;  // transitions
      static_cast<EbBase&>(inlet).contains(_contains);  // l1accepts
    }
  }

  unsigned partition= alloc.partitionid();

  //  setup EVR server
  Ins source(StreamPorts::event(partition, Level::Segment, _header.group()));
  Node evrNode(Level::Source,header().platform());
  evrNode.fixup(source.address(),Ether());
  DetInfo evrInfo(evrNode.pid(),DetInfo::NoDetector,0,DetInfo::Evr,0);
  EvrServer* esrv = new EvrServer(source, evrInfo, inlet, NetBufferDepth); // revisit
  inlet.add_input(esrv);
  esrv->server().join(source, Ins(header().ip()));
  printf("Assign evr %d  %x/%d\n",
   esrv->id(),source.address(),source.portId());
  _evr = esrv;

  // setup event servers
  unsigned nnodes   = alloc.nnodes();
  unsigned vectorid = 0;
  for (unsigned n=0; n<nnodes; n++) {
    const Node& node = *alloc.node(n);
    if (node.level()==Level::Event) {
      // Add vectored output clients on inlet
      Ins ins = StreamPorts::event(partition,
           Level::Event,
           vectorid,
           index);
      InletWireIns wireIns(vectorid, ins);
      inlet.add_output(wireIns);
      printf("SegmentLevel::allocated adding output %d to %x/%d\n",
       vectorid, ins.address(), ins.portId());
      vectorid++;
    }
  }

  OutletWire* owire = _streams->stream(StreamParams::FrameWork)->outlet()->wire();
  owire->bind(OutletWire::Bcast, StreamPorts::bcast(partition,
                Level::Event,
                index));

  //
  //  Assign traffic shaping phase
  //
  const int pid = getpid();
  for(unsigned n=0; n<nnodes; n++) {
    const Node& node = *alloc.node(n);
    if (node.level()==Level::Segment)
      if (node.pid()==pid) {
        ToEventWireScheduler::setPhase  (n % vectorid);
        ToEventWireScheduler::setMaximum(vectorid);
        ToEventWireScheduler::shapeTmo  (alloc.options()&Allocation::ShapeTmo);
        printf("Configure ToEventWireScheduler phase %d  max %d  tmo %c\n",
               n%vectorid, vectorid,
               alloc.options()&Allocation::ShapeTmo ? 't':'f');
      }
  }
}

void    SegmentLevel::dissolved()
{
  static_cast<InletWireServer*>(_streams->wire())->remove_input(_evr);
  static_cast<InletWireServer*>(_streams->wire())->remove_outputs();
}

void    SegmentLevel::post     (const Transition& tr)
{
  if (tr.id()!=TransitionId::L1Accept) {
    _streams->wire(StreamParams::FrameWork)->flush_inputs();
    _streams->wire(StreamParams::FrameWork)->flush_outputs();
  }
  _streams->wire(StreamParams::FrameWork)->post(tr);
}

void    SegmentLevel::post     (const Occurrence& tr)
{
  _streams->wire(StreamParams::FrameWork)->post(tr);
}

void    SegmentLevel::post     (const InDatagram& in)
{
  _streams->wire(StreamParams::FrameWork)->post(in);
}

void SegmentLevel::detach()
{
  if (_streams) {
    _streams->disconnect();
    delete _streams;
    _streams = 0;
    _callback.dissolved(header());
  }
  cancel();
}
