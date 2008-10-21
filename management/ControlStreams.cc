#include "ControlStreams.hh"
#include "pds/utility/ToEventWire.hh"
//#include "pds/utility/Occurrence.hh"
#include "pds/service/BitList.hh"
#include "pds/collection/Node.hh"
#include "pds/service/Task.hh"
#include "pds/service/VmonSourceId.hh"
#include "pds/collection/CollectionManager.hh"
#include "EventBuilder.hh"

using namespace Pds;

/*
class ControlOccHandler: public Appliance {
public:
  ControlOccHandler(ControlStreams& streams) : 
    _streams(streams),
    _synchs(sizeof(Occurrence), 1)
  {}

  virtual Datagram* occurrences(Datagram* in)
  {
    Occurrence* occ = (Occurrence*) in;
    switch (occ->type()) {
    case OccurrenceId::TrimRequest:
      _streams.trim_input(occ->reason());
      return 0;
    case OccurrenceId::SynchRequest:
      return new(&_synchs) Occurrence(OccurrenceId::Synch, 0);
    case OccurrenceId::Reboot:
    case OccurrenceId::Vmon:
      return in;
    default:
      return 0;
    }
  }
  virtual Datagram* transitions(Datagram* in) {return 0;}

private:
  ControlStreams& _streams;
  GenericPool _synchs;
};
*/

ControlStreams::ControlStreams(CollectionManager& cmgr) :
  WiredStreams(VmonSourceId(cmgr.header().level(), 0))
{
  //  VmonEb vmoneb(vmon());
  Level::Type level = cmgr.header().level();
  int ipaddress = cmgr.header().ip();
  unsigned eventpooldepth = 32;
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    _outlets[s] = new ToEventWire(*stream(s)->outlet(), 
				  cmgr,
				  ipaddress,
				  MaxSize*netbufdepth);
    _inlet_wires[s] = 
      new EventBuilder(Src(cmgr.header()),
		       TypeId(TypeNum::Id_InXtc),
		       level, *stream(s)->inlet(),
		       *_outlets[s], s, ipaddress,
		       MaxSize, eventpooldepth);
    //		vmoneb);
  }
  //  _occ_handler = new ControlOccHandler(*this);
  //  _occ_handler->connect(stream(StreamParams::Occurrence)->inlet());
}

ControlStreams::~ControlStreams() 
{
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    delete _inlet_wires[s];
    delete _outlets[s];
  }
  //  delete _occ_handler;
}

