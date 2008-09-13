#ifndef PDS_SEGWIRESETTINGS_HH
#define PDS_SEGWIRESETTINGS_HH

namespace Pds {

class Inlet;
class OutletWire;
class WiredStreams;
class InletWire;

class SegWireSettings {
public:
  virtual InletWire* create(Inlet& inlet,
			    OutletWire&, 
			    WiredStreams&, 
			    int stream) = 0;
};
}
#endif
