#include "ObserverStreams.hh"
#include "pds/utility/OpenOutlet.hh"
#include "pds/utility/ObserverEb.hh"
#include "pds/service/BitList.hh"
#include "pds/management/CollectionObserver.hh"
#include "pds/management/VmonServerAppliance.hh"
#include "pds/management/EventBuilder.hh"
#include "pds/service/VmonSourceId.hh"
#include "pds/vmon/VmonEb.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/utility/EbKGroup.hh"

#include <sys/types.h>  // required for kill
#include <signal.h>

using namespace Pds;

ObserverStreams::ObserverStreams(CollectionObserver& cmgr,
                                 int slowEb,
                                 unsigned max_size) :
  WiredStreams(VmonSourceId(cmgr.header().level(), cmgr.header().ip()))
{
  const Node& node = cmgr.header();
  Level::Type level = node.level();
  int ipaddress = node.ip();
  const Src& src = cmgr.header().procInfo();

  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {

    _outlets[s] = new OpenOutlet(*stream(s)->outlet());

    //ObserverEb* eb = new ObserverEb
    EbKGroup* eb = new EbKGroup
      (cmgr.header().procInfo(),
       _xtcType,
       level,
       *stream(s)->inlet(), *_outlets[s], s, ipaddress,
       max_size, ebdepth, slowEb,
       new VmonEb(src,32,ebdepth,(1<<23),max_size));

    _inlet_wires[s] = eb;

    (new VmonServerAppliance(src))->connect(stream(s)->inlet());
  }
}

ObserverStreams::~ObserverStreams()
{
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    delete _inlet_wires[s];
    delete _outlets[s];
  }
  //  delete _vmom_appliance;
}
