#include "InletWireIns.hh"

using namespace Pds;

InletWireIns::InletWireIns(unsigned id) :
  _id(id),
  _rcvr()
{}

InletWireIns::InletWireIns(unsigned id, const Ins& rcvr) :
  _id(id),
  _rcvr(rcvr)
{}

unsigned InletWireIns::id() const {return _id;}
const Ins& InletWireIns::rcvr() const {return _rcvr;}
