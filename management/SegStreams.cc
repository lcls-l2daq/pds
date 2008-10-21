#include "SegStreams.hh"
#include "EventBuilder.hh"
#include "pds/collection/CollectionManager.hh"
#include "pds/utility/ToEventWire.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/VmonSourceId.hh"
//#include "VmonAppliance.hh"

using namespace Pds;

static const unsigned MaxSize = 4*1024*1024;

SegStreams::SegStreams(CollectionManager& cmgr) :
  WiredStreams(VmonSourceId(cmgr.header().level(), cmgr.header().ip()))
  //  _vmom_appliance(new VmonAppliance(vmon()))
{
  Level::Type level  = cmgr.header().level();
  unsigned ipaddress = cmgr.header().ip();
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {

    _outlets[s] = new ToEventWire(*stream(s)->outlet(), 
				  cmgr, 
				  ipaddress, 
				  MaxSize*ebdepth);

    _inlet_wires[s] = 
      new EventBuilder(Src(cmgr.header()),
		       TypeId(TypeNum::Id_InXtc),
		       level,
		       *stream(s)->inlet(), *_outlets[s], s, ipaddress,
		       MaxSize, ebdepth);
  }
  //  _vmom_appliance->connect(stream(StreamParams::Occurrence)->inlet());
}

SegStreams::~SegStreams() 
{  
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    delete _inlet_wires[s];
    delete _outlets[s];
  }
  //  delete _vmom_appliance;
}
