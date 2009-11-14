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

EventStreams::EventStreams(PartitionMember& cmgr) :
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
				  MaxSize()*netbufdepth(),
				  cmgr.occurrences());

    _inlet_wires[s] = new EventBuilder(src,
				       _xtcType,
				       level,
				       *stream(s)->inlet(), 
				       *_outlets[s],
				       s,
				       ipaddress,
				       MaxSize(), EbDepth(),
				       new VmonEb(src,32,EbDepth(),(1<<23),(1<<22)));
				       
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

unsigned EventStreams::netbufdepth() const { return 8; }
unsigned EventStreams::EbDepth    () const { return 16; }
unsigned EventStreams::MaxSize    () const { return 16*1024*1024; }

