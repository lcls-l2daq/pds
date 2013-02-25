#include "EbIStream.hh"

#include "pds/utility/ToEb.hh"
#include "pds/utility/ToEbWire.hh"
#include "EventBuilder.hh"
#include "EventStreams.hh"
#include "pds/management/VmonServerAppliance.hh"
#include "pds/vmon/VmonEb.hh"
#include "pds/xtc/XtcType.hh"

using namespace Pds;

enum { MaxSize = 4*1024*1024 };
enum { EbDepth = 12 };

EbIStream::EbIStream(const Src&  src,
         int         interface,
         Level::Type level,
         InletWire&  inlet_wire,
         int         slowEb) :
  Stream(StreamParams::FrameWork)
{
  _output = new ToEb(src);
  _outlet_wire = new ToEbWire(*outlet(),
            inlet_wire,
            *_output);

  if (level == Level::Segment)
    _inlet_wire = new L1EventBuilder(src,
             _xtcType,
             level,
             *inlet(),
             *_outlet_wire,
             StreamParams::FrameWork,
             interface,
             MaxSize, EbDepth,
             slowEb,
             new VmonEb(src,32,EbDepth,(1<<24),(1<<22)));
  else
    _inlet_wire = new EventBuilder(src,
           _xtcType,
           level,
           *inlet(),
           *_outlet_wire,
           StreamParams::FrameWork,
           interface,
           MaxSize,
           EbDepth,
           slowEb,
           new VmonEb(src,32,EbDepth,(1<<23),(1<<22)));
  (new VmonServerAppliance(src))->connect(inlet());
}

EbIStream::~EbIStream()
{
  delete _inlet_wire;
  delete _outlet_wire;
}

InletWireServer* EbIStream::input() { return _inlet_wire; }

EbServer*       EbIStream::output() { return _output; }
