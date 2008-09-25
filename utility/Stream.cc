#include "Stream.hh"

using namespace Pds;

Stream::Stream(int stream) :
  _type(StreamParams::StreamType(stream)),
  _outlet()
{
  _outlet.connect(&_inlet);
}

StreamParams::StreamType Stream::type() const {return _type;}
Inlet* Stream::inlet() {return &_inlet;}
Outlet* Stream::outlet() {return &_outlet;}
