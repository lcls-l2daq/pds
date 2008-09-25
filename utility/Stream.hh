#ifndef PDS_STREAM_HH
#define PDS_STREAM_HH

#include "Inlet.hh"
#include "Outlet.hh"
#include "StreamParams.hh"

namespace Pds {

class Stream {
public:
  Stream(int stream);

  StreamParams::StreamType type() const;
  Inlet* inlet();
  Outlet* outlet();

private:
  StreamParams::StreamType _type;
  Inlet _inlet;
  Outlet _outlet;
};
}
#endif
