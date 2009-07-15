#ifndef PDS_TOEBWIRE_HH
#define PDS_TOEBWIRE_HH

#include "OutletWire.hh"
#include "InletWire.hh"

namespace Pds {

class ToEb;

class ToEbWire : public OutletWire {
public:
  ToEbWire(Outlet& outlet,
	   InletWire& inlet,
	   ToEb&   postman);
  virtual ~ToEbWire() {}
public:
  //  OutletWire interface
  virtual Transition* forward(Transition* dg);
  virtual Occurrence* forward(Occurrence* dg);
  virtual InDatagram* forward(InDatagram* dg);
  virtual void bind(NamedConnection, const Ins& node) {}
  virtual void bind(unsigned id, const Ins& node) {}
  virtual void unbind(unsigned id) {}
private:
  InletWire& _inlet;
  ToEb& _postman;
};
}
#endif
