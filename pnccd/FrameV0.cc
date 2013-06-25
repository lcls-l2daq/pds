/*
 * FrameV0.cc
 *
 *  Created on: May 9, 2013
 *      Author: jackp
 */

#include "pds/pnccd/FrameV0.hh"
#include "pdsdata/pnCCD/ConfigV2.hh"

using namespace Pds;
using namespace Pds::PNCCD;

uint32_t FrameV0::specialWord() { return _specialWord & 0xfffffff3; }

uint32_t FrameV0::frameNumber() { return _frameNumber; }

uint32_t FrameV0::timeStampHi() { return _timeStampHi; }

uint32_t FrameV0::timeStampLo() { return _timeStampLo; }

uint32_t FrameV0::elementId() { return _specialWord & 3; }

void FrameV0::elementId(uint32_t n) { _specialWord &= 0xfffffffc; _specialWord |= (n&3); }

void FrameV0::convertThisToFrameV1() { _specialWord &= 0xfffffffc; }

uint16_t* FrameV0::data() {return (uint16_t*)(this+1);}

unsigned FrameV0::sizeofData(ConfigV2& cfg) {
  return (cfg.payloadSizePerLink()-sizeof(*this))/sizeof(uint16_t);
}

FrameV0* FrameV0::next(ConfigV2& cfg) {
  return (FrameV0*)(((char*)this)+cfg.payloadSizePerLink());
}
