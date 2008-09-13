#include "InletWireIns.hh"

using namespace Pds;

InletWireIns::InletWireIns(unsigned id) :
  _id(id),
  _rcvr(),
  _mcast(0)
{}

InletWireIns::InletWireIns(unsigned id, const Ins& rcvr) :
  _id(id),
  _rcvr(rcvr),
  _mcast(0)
{}

InletWireIns::InletWireIns(unsigned id, const Ins& rcvr, int mcast) :
  _id(id),
  _rcvr(rcvr),
  _mcast(mcast)
{}

unsigned InletWireIns::id() const {return _id;}
const Ins& InletWireIns::rcvr() const {return _rcvr;}
int InletWireIns::mcast() const {return _mcast;}
