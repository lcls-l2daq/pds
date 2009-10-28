#include "ObserverStreams.hh"
#include "pds/utility/OpenOutlet.hh"
#include "EventBuilder.hh"
#include "pds/service/BitList.hh"
#include "pds/management/CollectionObserver.hh"
#include "pds/service/VmonSourceId.hh"
//#include "VmonAppliance.hh"
#include "pds/management/EventBuilder.hh"
#include "pds/xtc/XtcType.hh"

#include <sys/types.h>  // required for kill
#include <signal.h>

using namespace Pds;

ObserverStreams::ObserverStreams(CollectionObserver& cmgr) :
  WiredStreams(VmonSourceId(cmgr.header().level(), cmgr.header().ip()))
{
  const Node& node = cmgr.header();
  Level::Type level = node.level();
  int ipaddress = node.ip();

  //  VmonEb vmoneb(vmon());

   for (int s = 0; s < StreamParams::NumberOfStreams; s++) {

     _outlets[s] = new OpenOutlet(*stream(s)->outlet());

    EventBuilder* eb = new EventBuilder
      (cmgr.header().procInfo(),
       _xtcType,
       level,
       *stream(s)->inlet(), *_outlets[s], s, ipaddress,
       MaxSize, ebdepth);
    
    _inlet_wires[s] = eb;
  }
  //  _vmom_appliance = new VmonAppliance(vmon());
  //  _vmom_appliance->connect(stream(StreamParams::Occurrence)->inlet());
}

ObserverStreams::~ObserverStreams()
{
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    delete _inlet_wires[s];
    delete _outlets[s];
  }
  //  delete _vmom_appliance;
}
