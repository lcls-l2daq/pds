#include "SegStreams.hh"
#include "pds/utility/SegEventWire.hh"
//#include "SegEventWireZcp.hh"
//#include "VmonVebClient.hh"
#include "pds/utility/SegWireSettings.hh"
#include "pds/utility/InletWire.hh"
#include "pds/service/VmonSourceId.hh"
#include "pds/collection/CollectionManager.hh"
//#include "VmonAppliance.hh"

using namespace Pds;

SegStreams::SegStreams(const SegmentOptions& options,
		       SegWireSettings& settings, 
		       CollectionManager& collection) :
  //		       Gigmac* gb) :
  //  AckStreams(VmonSourceId(VmonSourceId::Segment, 
  //				crate, slot, detector), gb),
  //  _vmom_appliance(new VmonAppliance(vmon()))
  WiredStreams(collection,
	       VmonSourceId(VmonSourceId::Segment, collection.header().ip()))
{
  Src id(options.slot, options.module);
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    /***
    if (gb && s == StreamParams::FrameWork) {
      _outlets[s] = new SegEventWireZcp(*stream(s)->outlet(), ipaddress, 
					slot, crate, *_ack_handler, *gb);
    } else {
    ***/
    {
      _outlets[s] = new SegEventWire(*stream(s)->outlet(), collection.header().ip(), 
				     options.slot, options.crate); //, *_ack_handler);
    }
    _inlet_wires[s] = settings.create(*stream(s)->inlet(), *_outlets[s], 
				      *this, s);
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
