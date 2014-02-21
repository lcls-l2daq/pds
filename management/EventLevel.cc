#include <vector>

#include "EventLevel.hh"
#include "EventStreams.hh"
#include "EventCallback.hh"
#include "pds/collection/Message.hh"
#include "pds/utility/Transition.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/OutletWire.hh"
#include "pds/utility/InletWire.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/InletWireIns.hh"
#include "pds/utility/NetDgServer.hh"
#include "pds/utility/EbSGroup.hh"

using namespace Pds;

EventLevel::EventLevel(unsigned platform,
                       EventCallback& callback,
                       int slowEb,
                       Arp* arp,
                       unsigned max_eventsize,
                       unsigned max_buffers
                       ) :
  PartitionMember(platform, Level::Event, slowEb, arp),
  _callback   (callback),
  _streams    (0),
  _max_eventsize(max_eventsize),
  _max_buffers  (max_buffers)
{
  if (!_max_eventsize)
    _max_eventsize = EventStreams::MaxSize;
  if (!_max_buffers)
    _max_buffers   = EventStreams::EbDepth;
}

EventLevel::~EventLevel()
{
  if (_streams) delete _streams;
}

bool EventLevel::attach()
{
  start();
  while(1) {
    if (connect()) {
      if (_max_eventsize)
        _streams = new EventStreams(*this,
                                    slowEb(),
                                    _max_eventsize,
                                    EventStreams::netbufdepth,
                                    _max_buffers);
      else
        _streams = new EventStreams(*this, slowEb());

      _streams->connect();

      _callback.attached(*_streams);

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

void EventLevel::dissolved()
{
  _streams->disconnect();
  _streams->connect();
}

void EventLevel::detach()
{
  if (_streams) {
    _streams->disconnect();
    delete _streams;
    _streams = 0;
    //    _callback.dissolved(_dissolver);
  }
  cancel();
}

Message& EventLevel::reply(Message::Type)
{
  return _reply;
}

void    EventLevel::allocated(const Allocation& alloc,
            unsigned          index)
{
  InletWire* inlet = _streams->wire(StreamParams::FrameWork);


  //!!! segment group support
  std::vector<EbBitMask> lGroupSegMask;

  // setup BLD and event servers
  unsigned partition  = alloc.partitionid();
  unsigned nnodes     = alloc.nnodes();
  unsigned recorderid = 0;
  unsigned segmentid  = 0;
  for (unsigned n=0; n<nnodes; n++) {
    const Node& node = *alloc.node(n);
    if (node.level() == Level::Segment) {
      // Add vectored output clients on bld_wire
      Ins ins = StreamPorts::event(partition,
           Level::Event,
           index,
           segmentid);

      //!!! segment group support
      if (node.group() >= lGroupSegMask.size())
        lGroupSegMask.resize(node.group()+1);
      lGroupSegMask[node.group()].setBit(segmentid);

      ++segmentid;

      Ins srvIns(ins.portId());
      NetDgServer* srv = new NetDgServer(srvIns,
           node.procInfo(),
           EventStreams::netbufdepth*EventStreams::MaxSize);
      inlet->add_input(srv);
      Ins mcastIns(ins.address());
      srv->server().join(mcastIns, Ins(header().ip()));
      Ins bcastIns = StreamPorts::bcast(partition, Level::Event);
      srv->server().join(bcastIns, Ins(header().ip()));
      printf("EventLevel::allocated assign fragment %d  src %x/%d  dst %x/%d  fd %d\n",
       srv->id(),
       node.procInfo().ipAddr(),node.procInfo().processId(),
       mcastIns.address(),srv->server().portId(),
       srv->fd());
    }
    else if (node.level() == Level::Control) {
      Ins ins = StreamPorts::event(partition,
                 Level::Control,
                 recorderid,
                 index);
      InletWireIns wireIns(recorderid, ins);
      inlet->add_output(wireIns);
      printf("EventLevel::message adding output %d to %x/%d\n",
             recorderid, ins.address(), ins.portId());
      recorderid++;
    }
  } // for (unsigned n=0; n<nnodes; n++)

  //!!! segment group support
  for (int iGroup = 0; iGroup < (int) lGroupSegMask.size(); ++iGroup)
    printf("Group %d Segment Mask 0x%04x%04x\n", iGroup, lGroupSegMask[iGroup].value(1), lGroupSegMask[iGroup].value(0) );
  ((EbSGroup*) inlet)-> setClientMask(lGroupSegMask);

  OutletWire* owire = _streams->stream(StreamParams::FrameWork)->outlet()->wire();
  owire->bind(OutletWire::Bcast, StreamPorts::bcast(partition,
                Level::Control,
                index));
}

void    EventLevel::post     (const Transition& tr)
{
  if (tr.id()!=TransitionId::L1Accept) {
    _streams->wire(StreamParams::FrameWork)->flush_inputs();
    _streams->wire(StreamParams::FrameWork)->flush_outputs();
  }
  _streams->wire(StreamParams::FrameWork)->post(tr);
}

void    EventLevel::post     (const Occurrence& tr)
{
  _streams->wire(StreamParams::FrameWork)->post(tr);
}

void    EventLevel::post     (const InDatagram& in)
{
  _streams->wire(StreamParams::FrameWork)->post(in);
}

#if 0 // revisit
void EventLevel::disconnected()
{
}
#endif
