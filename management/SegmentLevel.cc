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
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/management/EventBuilder.hh"
#include "pds/management/EventCallback.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <unistd.h>
#include <stdlib.h>

using namespace Pds;

static const unsigned NetBufferDepth = 1024;

SegmentLevel::SegmentLevel(unsigned         platform,
                           SegWireSettings& settings,
                           EventCallback&   callback ) :
  PartitionMember(platform, Level::Segment),
  _settings      (settings),
  _callback      (callback),
  _streams       (0),
  _evr           (0),
  _reply         (settings.sources(), settings.max_event_size())
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
      const char* vname = 0;
      if (_settings.pAliases() && !_settings.pAliases()->empty())
        vname = _settings.pAliases()->front().aliasName();
      else if (!_settings.sources().empty())
        vname = DetInfo::name(static_cast<const DetInfo&>(_settings.sources().front()));

      if (_settings.needs_evr() && _settings.is_triggered())
	_header.setTrigger(_settings.module(),_settings.channel());

      _header.setPaddr  (_settings.paddr());
      _header.setPayload(_settings.max_event_size());

      _streams = new SegStreams(*this,
                                _settings,
                                vname);
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
      exit(1);
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

  //
  //  Group support may transition into multiple partitions
  //

  //!!! segment group support
//   for (unsigned n=0; n<alloc.nnodes(); n++) {
//     const Node& node = *alloc.node(n);
//     if (node.level() == Level::Segment &&
//         node == _header) {
//       if (node.group() != _header.group())
//         {
//           _header.setGroup(node.group());
//           printf("Assign group to %d\n", _header.group());
//         }
      
//       _contains = node.transient()?_transientXtcType:_xtcType;  // transitions
//       static_cast<EbBase&>(inlet).contains(_contains);  // l1accepts
//     }
//   }

  unsigned partition= alloc.partitionid();

  if (_settings.needs_evr()) {
    //  setup EVR server
    Ins source(StreamPorts::event(partition, Level::Segment, _header.group(), 
                                  _header.triggered() ? _header.evr_module() : alloc.masterid()));
    //Ins source(StreamPorts::event(partition, Level::Segment, _header.group(), alloc.masterid()));
    Node evrNode(Level::Source,header().platform());
    evrNode.fixup(source.address(),Ether());
    DetInfo evrInfo(evrNode.pid(),DetInfo::NoDetector,0,DetInfo::Evr,0);
    EvrServer* esrv = new EvrServer(source, evrInfo, 
                                    static_cast<InletWireServer&>(inlet)._inlet,
                                    NetBufferDepth); // revisit
    inlet.add_input(esrv);
    esrv->server().join(source, Ins(header().ip()));
    printf("Assign evr %d  (trigger %d masterid %d module %d) %x/%d\n",
           esrv->id(), _header.triggered(), alloc.masterid(), _header.evr_module(), source.address(),source.portId());
    _evr = esrv;
  }

  OutletWire* owire = _streams->stream(StreamParams::FrameWork)->outlet()->wire();
  owire->bind(OutletWire::Bcast, StreamPorts::bcast(partition,
                                                    Level::Control,
                                                    index));
}

void    SegmentLevel::dissolved()
{
  if (_evr) {
    static_cast<InletWireServer*>(_streams->wire())->remove_input(_evr);
    _evr=0;
  }
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
  //
  //  Add DetInfo advertisement on Map
  //
  if (in.seq.service()==TransitionId::Map) {
    const std::list<Src>& sources = _settings.sources();
    char* pool = new char[sizeof(Xtc)*64];
    Xtc* tc = new(pool) Xtc(_transientXtcType,header().procInfo());
    for(std::list<Src>::const_iterator it=sources.begin(); it!=sources.end(); it++)
      new (tc) Xtc(_transientXtcType,(*it));
    const_cast<InDatagram&>(in).insert(*tc,tc->payload());
    delete[] pool;
  }
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
