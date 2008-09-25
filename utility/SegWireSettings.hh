#ifndef PDS_SEGWIRESETTINGS_HH
#define PDS_SEGWIRESETTINGS_HH

namespace Pds {

class InletWire;

class SegWireSettings {
public:
  virtual void connect (InletWire& inlet,
			StreamParams::StreamType stream,
			int interface) = 0;
};
}
#endif
