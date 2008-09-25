#ifndef PDS_OUTLET_HH
#define PDS_OUTLET_HH

#include "Appliance.hh"
#include "OccurrenceId.hh"
#include "EventId.hh"
#include "MarkerId.hh"
#include "pds/xtc/Sequence.hh"

namespace Pds {

class OutletWire;

//
// An outlet is an appliance which is responsible for sending datagrams on
// streams across level boundaries. All incoming datagrams are checked to
// see which level (if any they should be sent to). These are then transmitted
// using the OutletWire which was passed as a constructor argument. The
// datagrams are then deleted.
//

class Outlet: public Appliance {

public:
  Outlet();
  virtual ~Outlet() {}

  void plug(OutletWire& wire);
  OutletWire* wire();

  virtual Transition* transitions(Transition*);
  virtual InDatagram* events     (InDatagram*);
  virtual InDatagram* occurrences(InDatagram*);
  virtual InDatagram* markers    (InDatagram*);

  void forward(OccurrenceId::Value id);
  void forward(EventId::Value id);
  void forward(MarkerId::Value id);
  void forward(Sequence::Type type, unsigned mask);

  void sink(OccurrenceId::Value id);
  void sink(EventId::Value id);
  void sink(MarkerId::Value id);
  void sink(Sequence::Type type, unsigned mask);

  unsigned forward(Sequence::Type type) const;

private:
  unsigned _forward[Sequence::NumberOfTypes];
  OutletWire* _wire;
};
}
#endif
