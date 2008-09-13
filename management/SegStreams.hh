#ifndef ODFSEGSTREAMS_HH
#define ODFSEGSTREAMS_HH

//#include "AckStreams.hh"
#include "pds/utility/WiredStreams.hh"

namespace Pds {

class SegWireSettings;
// class VmonAppliance;

class SegmentOptions {
public:
  unsigned detector;
  unsigned module;
  unsigned crate;
  unsigned slot;
};


//class SegStreams : public AckStreams {
class SegStreams : public WiredStreams {
public:
  SegStreams(const SegmentOptions& options,
	     SegWireSettings& settings,
	     CollectionManager&);

  virtual ~SegStreams();

  friend class SegmentLevel;
  friend class FragmentLevel;

private:
  //  VmonAppliance* _vmom_appliance;
};

}
#endif
