#include "TaggedStreams.hh"
#include "EventBuilder.hh"
#include "pds/management/PartitionMember.hh"
#include "pds/management/VmonServerAppliance.hh"
#include "pds/utility/ToEventWireScheduler.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/VmonSourceId.hh"
#include "pds/vmon/VmonEb.hh"
#include "pds/xtc/XtcType.hh"

using namespace Pds;

static const unsigned MaxSize = 16*1024*1024;

TaggedStreams::TaggedStreams(PartitionMember& cmgr,
                             unsigned max_event_size,
                             unsigned max_event_depth,
                             const char* name) :
  WiredStreams(VmonSourceId(cmgr.header().level(), cmgr.header().ip()))
{
  if (!max_event_size)
    max_event_size = MaxSize;

  const Node& node = cmgr.header();
  Level::Type level  = node.level();
  unsigned ipaddress = node.ip();
  const Src& src = node.procInfo();
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {

    _outlets[s] = new ToEventWireScheduler(*stream(s)->outlet(),
                                           cmgr,
                                           ipaddress,
                                           max_event_size*max_event_depth,
                                           cmgr.occurrences());

    EbS* ebs = new EbS(src,
                       _xtcType,
                       level,
                       *stream(s)->inlet(),
                       *_outlets[s],
                       s,
                       ipaddress,
                       max_event_size, max_event_depth, 0,
                       new VmonEb(src,32,max_event_depth,(1<<23),max_event_size));
    ebs->require_in_order(true);
    ebs->printSinks(false); // these are routine
    _inlet_wires[s] = ebs;

    (new VmonServerAppliance(src,name))->connect(stream(s)->inlet());
  }
}

TaggedStreams::~TaggedStreams()
{
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    delete _inlet_wires[s];
    delete _outlets[s];
  }
}
