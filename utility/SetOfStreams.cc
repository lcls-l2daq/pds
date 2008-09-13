#include "SetOfStreams.hh"
#include "InletWire.hh"
#include "OutletWire.hh"
//#include "VmonManager.hh"

using namespace Pds;

SetOfStreams::SetOfStreams(CollectionManager& cmgr,
			   const VmonSourceId& vmonid)
{
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    _stream[s] = new Stream(s,cmgr);
    _inlet_wires[s] = 0;
  }
  //  _vmon_manager = new VmonManager(vmonid);
}

SetOfStreams::~SetOfStreams()
{
  //  delete _vmon_manager;
  for (int s = 0; s < StreamParams::NumberOfStreams; s++) {
    delete _stream[s];
  }
}

InletWire* SetOfStreams::wire(int s) {return _inlet_wires[s];}

Stream* SetOfStreams::stream(int s) {return _stream[s];}

//VmonManager& SetOfStreams::vmon() {return *_vmon_manager;}

void SetOfStreams::dump(int stream, int detail = 1) 
{  
  _inlet_wires[stream]->dump(detail);
  _outlets[stream]->dump(detail);
}

void SetOfStreams::dumpCounters(int stream, int detail = 1) 
{  
  _inlet_wires[stream]->dumpCounters(detail);
  _outlets[stream]->dumpCounters(detail);
}

void SetOfStreams::resetCounters(int stream) 
{  
  _inlet_wires[stream]->resetCounters();
  _outlets[stream]->resetCounters();
}

void SetOfStreams::dumpHistograms(int stream, unsigned tag, 
				     const char* path) 
{  
  _inlet_wires[stream]->dumpHistograms(tag, path);
  _outlets[stream]->dumpHistograms(tag, path);
}

void SetOfStreams::resetHistograms(int stream) 
{  
  _inlet_wires[stream]->resetHistograms();
  _outlets[stream]->resetHistograms();
}

