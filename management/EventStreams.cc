#include "EventStreams.hh"
#include "pds/utility/ToEventWire.hh"
#include "EventBuilder.hh"
#include "pds/service/BitList.hh"
#include "pds/collection/CollectionManager.hh"
#include "pds/service/VmonSourceId.hh"
//#include "VmonAppliance.hh"
//#include "pds/collection/McastDb.hh"
//#include "pds/collection/BcastRegistery.hh"

#include <sys/types.h>  // required for kill
#include <signal.h>

using namespace Pds;

EventStreams::EventStreams(CollectionManager& cmgr) :
  WiredStreams(VmonSourceId(cmgr.header().level(), cmgr.header().ip()))
{
  const Node& node = cmgr.header();
  Level::Type level = node.level();
  int ipaddress = node.ip();

  //  VmonEb vmoneb(vmon());

  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {

    _outlets[s] = new ToEventWire(*stream(s)->outlet(), cmgr, ipaddress);

    if (cmgr.header().level()==Level::Recorder) {
      EventBuilder* eb = new EventBuilder
	(Src(cmgr.header()),
	 level,
	 *stream(s)->inlet(), *_outlets[s], s, ipaddress,
	 MaxSize, ebdepth);
      
      eb->no_build(Sequence::Event,1<<TransitionId::L1Accept);
      _inlet_wires[s] = eb;
    }
    else {
      EventBuilder* eb = new EventBuilder
	(Src(cmgr.header()),
	 level,
	 *stream(s)->inlet(), *_outlets[s], s, ipaddress,
	 MaxSize, ebdepth);
      _inlet_wires[s] = eb;
    }

  }
  //  _vmom_appliance = new VmonAppliance(vmon());
  //  _vmom_appliance->connect(stream(StreamParams::Occurrence)->inlet());
}

EventStreams::~EventStreams()
{
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    delete _inlet_wires[s];
    delete _outlets[s];
  }
  //  delete _vmom_appliance;
}
