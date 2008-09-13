#ifndef PDS_TOEVENTWIRE_HH
#define PDS_TOEVENTWIRE_HH

#include <sys/uio.h>

#include "OutletWire.hh"

#include "OutletWireInsList.hh"
#include "pds/xtc/xtc.hh"

namespace Pds {

class Outlet;
class Datagram;
//class AckHandler;
//class AckRequest;

class ToEventWire : public OutletWire {
public:
  ToEventWire(Outlet& outlet, 
	      unsigned slot,
	      unsigned clients,
	      const Src& src);
	      //	      AckHandler& ack_handler);
  ~ToEventWire();

  virtual void bind(unsigned id, const Ins& node, int mcast);
  virtual void unbind(unsigned id);

  // Debugging
  virtual void dump(int detail);
  virtual void dumpHistograms(unsigned tag, const char* path);
  virtual void resetHistograms();

protected:
  OutletWireIns* prepareEvent   (const Datagram&);
  //				 AckRequest*&);

  bool isempty() const {return _nodes.isempty();}

private:
  OutletWireInsList _nodes;
  OutletWireIns     _mcast;
  //  AckHandler& _ack_handler;
  Src _id;
};
}

#endif
