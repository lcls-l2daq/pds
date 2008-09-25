#ifndef PDS_SETOFSTREAMS_HH
#define PDS_SETOFSTREAMS_HH

#include "Stream.hh"

namespace Pds {

class InletWire;
class OutletWire;
class VmonSourceId;
//class VmonManager;

class SetOfStreams {
public:
  SetOfStreams(const VmonSourceId& vmonid);

  // Accessors
  InletWire* wire(int s=StreamParams::FrameWork);
  Stream* stream(int s=StreamParams::FrameWork);
  //  VmonManager& vmon();

  // Debugging
  void dump(int stream, int detail);
  void dumpCounters(int stream, int detail);
  void resetCounters(int stream);
  void dumpHistograms(int stream, unsigned tag, const char* path);
  void resetHistograms(int stream);

protected:
  virtual ~SetOfStreams();

protected:
  InletWire* _inlet_wires[StreamParams::NumberOfStreams];
  OutletWire* _outlets[StreamParams::NumberOfStreams];

private:
  Stream* _stream[StreamParams::NumberOfStreams];
  //  VmonManager* _vmon_manager;
};
}
#endif
