#include "EventStreams.hh"
#include "EventBuilder.hh"
#include "pds/utility/ToEventWire.hh"
#include "pds/management/PartitionMember.hh"
#include "pds/management/VmonServerAppliance.hh"
#include "pds/service/VmonSourceId.hh"
#include "pds/service/BitList.hh"
#include "pds/vmon/VmonEb.hh"
#include "pds/xtc/XtcType.hh"

#include <sys/types.h>  // required for kill
#include <signal.h>

using namespace Pds;

EventStreams::EventStreams(PartitionMember& cmgr,
			   unsigned max_size,
			   unsigned net_buf_depth,
			   unsigned eb_depth) :
  WiredStreams(VmonSourceId(cmgr.header().level(), cmgr.header().ip()))
{
  const Node& node = cmgr.header();
  Level::Type level = node.level();
  int ipaddress = node.ip();
  const Src& src = cmgr.header().procInfo();
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {

    _outlets[s] = new ToEventWire(*stream(s)->outlet(), 
				  cmgr, 
				  ipaddress, 
				  max_size*net_buf_depth,
				  cmgr.occurrences());

    _inlet_wires[s] = new EventBuilder(src,
				       _xtcType,
				       level,
				       *stream(s)->inlet(), 
				       *_outlets[s],
				       s,
				       ipaddress,
				       max_size, eb_depth,
				       new VmonEb(src,32,eb_depth,(1<<23),(1<<22)));
				       
    (new VmonServerAppliance(src))->connect(stream(s)->inlet());
  }
}

EventStreams::~EventStreams()
{
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    delete _inlet_wires[s];
    delete _outlets[s];
  }
}


