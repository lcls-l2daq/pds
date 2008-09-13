#include "EventStreams.hh"
#include "pds/utility/EventOutletWire.hh"
#include "pds/utility/Eb.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/BitList.hh"
#include "pds/collection/CollectionManager.hh"
#include "pds/service/VmonSourceId.hh"
//#include "VmonAppliance.hh"
//#include "pds/collection/McastDb.hh"
//#include "pds/collection/BcastRegistery.hh"

#include <sys/types.h>  // required for kill
#include <signal.h>

namespace Pds {

class EventOccHandler: public Appliance {
public:
  EventOccHandler(unsigned id) : _id(id) {}

  virtual Datagram* occurrences(Datagram* in)
  {
    Occurrence* dg = (Occurrence*) in;
    if (dg->type() == OccurrenceId::TrimRequest) {
      if (dg->reason() == _id) {
	// Used to call partition->dissolve() here, but that hangs if
	// the framework stream is hung (not uncommon if we're
	// trimming). So, since things may be hung, punt on an orderly
	// shutdown and set off the nuclear device. - cpo
	printf("*** EventOccHandler trim request at time 0x%6.6x/%8.8x\n", 
	       in->high(), in->low());
	kill(0, SIGABRT);    // send abort signal to all processes in process group
      }
    }
    return in;
  }
  virtual Datagram* events(Datagram* in) {return in;}

private:
  unsigned _id;
};
}

using namespace Pds;

EventStreams::EventStreams(CollectionManager& cmgr) :
  WiredStreams(cmgr, VmonSourceId(VmonSourceId::Event, cmgr.header().ip()))
{
  const unsigned MaxSize = 0x400000;  // 4 MB!
  const unsigned MaxContributions = 10;

  const Node& node = cmgr.header();

  //  VmonEb vmoneb(vmon());
  //  unsigned crates = node.crates();
  int ipaddress = node.ip();
  unsigned ncrates = MaxContributions;
  unsigned eventpooldepth = ncrates+1; // need to allow dropped segments (+1)
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    StreamParams::StreamType type = StreamParams::StreamType(s);
    //    EbTimeouts ebtimeouts(crates, type, Level::Event);
    EbTimeouts ebtimeouts(1, type, Level::Event);
    _outlets[s] = 
      new EventOutletWire(*stream(s)->outlet(), s, ipaddress);
    unsigned eventsize = (s == StreamParams::FrameWork ?
			  MaxSize : MaxContributions*sizeof(Occurrence));
    //    Ins dstack(McastDb::acks(crates),
    //		  BcastRegistery::ack(node.platform()));
    _inlet_wires[s] = 
      new Eb(Src(ipaddress, node.partition()), Level::Event, 
	     *stream(s)->inlet(), *_outlets[s], s, ipaddress,
	     eventsize, eventpooldepth, netbufdepth, 
	     ebtimeouts );
    //	     vmoneb, &dstack);
  }
  //  _occ_handler = new EventOccHandler(id);
  //  _occ_handler->connect(stream(StreamParams::Occurrence)->inlet());
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
  delete _occ_handler;
}
