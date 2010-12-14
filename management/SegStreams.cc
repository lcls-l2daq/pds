#include "SegStreams.hh"
#include "EventBuilder.hh"
#include "pds/management/PartitionMember.hh"
#include "pds/management/VmonServerAppliance.hh"
#include "pds/utility/ToEventWireScheduler.hh"
#include "pds/utility/ToEventWire.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/VmonSourceId.hh"
#include "pds/vmon/VmonEb.hh"
#include "pds/xtc/XtcType.hh"

using namespace Pds;

static const unsigned MaxSize = 5*1024*1024;

SegStreams::SegStreams(PartitionMember& cmgr) :
  WiredStreams(VmonSourceId(cmgr.header().level(), cmgr.header().ip()))
{
  const Node& node = cmgr.header();
  Level::Type level  = node.level();
  unsigned ipaddress = node.ip();
  const Src& src = node.procInfo();
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {

    _outlets[s] = new ToEventWireScheduler(*stream(s)->outlet(), 
					   //    _outlets[s] = new ToEventWire(*stream(s)->outlet(), 
				  cmgr, 
				  ipaddress, 
				  MaxSize*ebdepth,
				  cmgr.occurrences());

    _inlet_wires[s] = new L1EventBuilder(src,
					 _xtcType,
					 level,
					 *stream(s)->inlet(),
					 *_outlets[s],
					 s,
					 ipaddress,
					 MaxSize, ebdepth,
					 new VmonEb(src,32,ebdepth,(1<<24),(1<<22)));

    (new VmonServerAppliance(src))->connect(stream(s)->inlet());
  }
}

SegStreams::~SegStreams() 
{  
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    delete _inlet_wires[s];
    delete _outlets[s];
  }
}
