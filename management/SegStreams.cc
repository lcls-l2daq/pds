#include "pds/management/SegStreams.hh"
#include "pds/management/EventBuilder.hh"
#include "pds/management/PartitionMember.hh"
#include "pds/management/VmonServerAppliance.hh"
#include "pds/utility/SegOutlet.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/VmonSourceId.hh"
#include "pds/vmon/VmonEb.hh"
#include "pds/xtc/XtcType.hh"

using namespace Pds;

SegStreams::SegStreams(PartitionMember& cmgr,
                       const SegWireSettings& settings,
                       const char* name) :
  WiredStreams(VmonSourceId(cmgr.header().level(), cmgr.header().ip()))
{
  const Node& node   = cmgr.header();
  Level::Type level  = node.level();
  unsigned ipaddress = node.ip();
  const Src& src     = node.procInfo();
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {

    _outlets[s] = new SegOutlet(*stream(s)->outlet(),
                                cmgr,
                                cmgr.occurrences());

    VmonEb* vmon = new VmonEb(src,2,
                              settings.max_event_depth(),
                              settings.max_event_time (),
                              settings.max_event_size ());

    if (settings.needs_evr())
      _inlet_wires[s] = new EbC(src,
                                _xtcType,
                                level,
                                *stream(s)->inlet(),
                                *_outlets[s],
                                s,
                                ipaddress,
                                settings.max_event_size(),
                                settings.max_event_depth(),
                                vmon);
    else
      _inlet_wires[s] = new EbS(src,
                                _xtcType,
                                level,
                                *stream(s)->inlet(),
                                *_outlets[s],
                                s,
                                ipaddress,
                                settings.max_event_size(),
                                settings.max_event_depth(),
                                vmon);
    
    (new VmonServerAppliance(src,name))->connect(stream(s)->inlet());
  }
}

SegStreams::~SegStreams()
{
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    delete _inlet_wires[s];
    delete _outlets[s];
  }
}
