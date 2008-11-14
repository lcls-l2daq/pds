#ifndef Pds_EbIStream_hh
#define Pds_EbIStream_hh

#include "pds/utility/Stream.hh"
#include "pds/utility/StreamParams.hh"
#include "pdsdata/xtc/Level.hh"

namespace Pds {

  class InletWire;
  class InletWireServer;
  class EbServer;
  class EbC;
  class OutletWire;
  class ToEb;

  class EbIStream : public Stream {
    enum { MaxSize = 4*1024*1024 };
    enum { EbDepth = 8 };
    enum { NetDepth = 8 };
  public:
    EbIStream(const Src& src,
	      int        interface,
	      Level::Type level,
	      InletWire& inlet_wire);
    virtual ~EbIStream();

    InletWireServer* input ();
    EbServer*        output();
  private:
    InletWireServer* _inlet_wire;
    OutletWire*      _outlet_wire;
    ToEb*            _output;
  };
}

#endif
