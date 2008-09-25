#ifndef PDS_WIREDSTREAMS_HH
#define PDS_WIREDSTREAMS_HH

#include "SetOfStreams.hh"
#include "StreamRcvr.hh"
#include "pds/service/EbBitMaskArray.hh"

namespace Pds {

class WiredStreams : public SetOfStreams {
public:
  WiredStreams(const VmonSourceId& vmonid);
  virtual ~WiredStreams();

  // Connecting wires
  void connect();
  void disconnect();
};
}
#endif
