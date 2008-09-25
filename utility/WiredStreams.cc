#include "WiredStreams.hh"
#include "InletWire.hh"
#include "InletWireIns.hh"
#include "OutletWire.hh"

using namespace Pds;

WiredStreams::WiredStreams(const VmonSourceId& vmonid) :
  SetOfStreams(vmonid)
{}

WiredStreams::~WiredStreams() {}

void WiredStreams::connect() 
{
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    _inlet_wires[s]->connect();
  }
}

void WiredStreams::disconnect() 
{
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    _inlet_wires[s]->disconnect();
  }
}

