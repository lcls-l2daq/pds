#include "EbIStream.hh"

#include "pds/utility/EbC.hh"
#include "pds/utility/EbS.hh"
#include "pds/utility/ToEb.hh"
#include "pds/utility/ToEbWire.hh"

using namespace Pds;


EbIStream::EbIStream(const Src&  src,
		     int         interface,
		     Level::Type level,
		     InletWire&  inlet_wire) :
  Stream(StreamParams::FrameWork)
{
  _output = new ToEb(src);
  _outlet_wire = new ToEbWire(*outlet(),
			      inlet_wire,
			      *_output);

  if (level == Level::Segment)
    _inlet_wire = new EbC(src, 
			  level,
			  *inlet(), 
			  *_outlet_wire, 
			  StreamParams::FrameWork, 
			  interface,
			  MaxSize, EbDepth);
  else
    _inlet_wire = new EbS(src, 
			  level,
			  *inlet(), 
			  *_outlet_wire, 
			  StreamParams::FrameWork, 
			  interface,
			  MaxSize, EbDepth);
}
 
EbIStream::~EbIStream()
{
  delete _inlet_wire;
  delete _outlet_wire;
}

InletWireServer* EbIStream::input() { return _inlet_wire; }

EbServer*       EbIStream::output() { return _output; }
