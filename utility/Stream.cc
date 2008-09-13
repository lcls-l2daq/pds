#include "Stream.hh"

using namespace Pds;

Stream::Stream(int stream, 
	       CollectionManager& cmgr) :
  _type(StreamParams::StreamType(stream)),
  _outlet(cmgr)
{
  _outlet.connect(&_inlet);
}

StreamParams::StreamType Stream::type() const {return _type;}
Inlet* Stream::inlet() {return &_inlet;}
Outlet* Stream::outlet() {return &_outlet;}
