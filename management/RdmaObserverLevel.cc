//
//  Add Rdma support
//  Remove group support
//
#include <vector>

#include "RdmaObserverLevel.hh"
#include "ObserverStreams.hh"
#include "EventCallback.hh"
#include "pds/collection/Message.hh"
#include "pds/utility/Transition.hh"
#include "pds/utility/StreamPorts.hh"
#include "pds/utility/InletWire.hh"
#include "pds/utility/InletWireServer.hh"
#include "pds/utility/InletWireIns.hh"
#include "pds/utility/OpenOutlet.hh"
#include "pds/utility/RdmaDgServer.hh"

using namespace Pds;

RdmaObserverLevel::RdmaObserverLevel(unsigned       platform,
                                     const char*    partition,
                                     unsigned       nodes,
                                     EventCallback& callback) :
  CollectionObserver(platform, partition),
  _nodes            (nodes),
  _callback         (callback),
  _streams          (0),
  _max_eventsize    (max_eventsize)
{
}

RdmaObserverLevel::~RdmaObserverLevel()
{
  if (_streams) {
    for (int s = 0; s < StreamParams::NumberOfStreams; s++)
      delete _outlets[s];
  }
}

bool RdmaObserverLevel::attach()
{
  start();
  if (connect()) {
    if (_max_eventsize)
      _streams = new ObserverStreams(*this, _max_eventsize);
    else
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

void RdmaObserverLevel::allocated(const Allocation& alloc)
{
  InletWire* inlet = _streams->wire(StreamParams::FrameWork);

  // setup BLD and event servers
  unsigned partition  = alloc.partitionid();
  unsigned nnodes     = alloc.nnodes();
  unsigned segmentid  = 0;
  uint64_t offset     = 0;
  for (unsigned n=0; n<nnodes; n++) {
    const Node& node = *alloc.node(n);
    if (node.level() == Level::Segment) {
      Ins srvIns(node.ip(),
                 StreamPorts::event(partition,
                                    Level::Event,
                                    0,  // this parameter is ignored for the port server assignment
                                    segmentid).portId());

      RdmaDgServer* srv = new RdmaDgServer(srvIns,
                                           offset,
                                           node.payload());
      inlet->add_input(srv);
      offset = srv->next_payload();

      segmentid++;
    } // if (node.level() == Level::Segment) {
  } // for (unsigned n=0; n<nnodes; n++) {
}

void RdmaObserverLevel::post     (const Transition& tr)
{
  if (tr.id()==TransitionId::Disable)
    _streams->wire(StreamParams::FrameWork)->flush_inputs();

  _streams->wire(StreamParams::FrameWork)->post(tr);
}

void RdmaObserverLevel::post     (const InDatagram& in)
{
  _streams->wire(StreamParams::FrameWork)->post(in);
}

void RdmaObserverLevel::dissolved()
{
  _streams->disconnect();
  _streams->connect();
}

void RdmaObserverLevel::detach()
{
  if (_streams) {
    _streams->disconnect();
    delete _streams;
    _streams = 0;
    //    _callback.dissolved(_dissolver);
  }
  cancel();
}

WiredStreams* RdmaObserverLevel::streams() { return _streams; }
