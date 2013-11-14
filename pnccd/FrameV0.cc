/*
 * FrameV0.cc
 *
 *  Created on: May 9, 2013
 *      Author: jackp
 */

#include "pds/pnccd/FrameV0.hh"
#include "pdsdata/psddl/pnccd.ddl.h"

using namespace Pds;
using namespace Pds::PNCCD;

static int _shuffle(const void* invoid, void* outvoid, unsigned int nelements);

uint32_t FrameV0::specialWord() const { return _specialWord & 0xfffffff3; }

uint32_t FrameV0::frameNumber() const { return _frameNumber; }

uint32_t FrameV0::timeStampHi() const { return _timeStampHi; }

uint32_t FrameV0::timeStampLo() const { return _timeStampLo; }

uint32_t FrameV0::elementId() const { return _specialWord & 3; }

void FrameV0::elementId(uint32_t n) { _specialWord &= 0xfffffffc; _specialWord |= (n&3); }

void FrameV0::convertThisToFrameV1() { _specialWord &= 0xfffffffc; }

const uint16_t* FrameV0::data() const {return (const uint16_t*)(this+1);}

unsigned FrameV0::sizeofData(ConfigV2& cfg) const {
  return (cfg.payloadSizePerLink()-sizeof(*this))/sizeof(uint16_t);
}

FrameV0* FrameV0::next(const ConfigV2& cfg) const {
  return (FrameV0*)(((char*)this)+cfg.payloadSizePerLink());
}

void FrameV0::shuffle(void* out) const {
  _shuffle((const void*)data(), out, sizeof(ImageQuadrant)/sizeof(uint16_t));
}

#define XA0(x)  (((x) & 0x000000000000ffffull))

#define XA1(x)  (((x) & 0x000000000000ffffull) << 16)

#define XA2(x)  (((x) & 0x000000000000ffffull) << 32)

#define XA3(x)  (((x) & 0x000000000000ffffull) << 48)

#define XB0(x)  (((x) & 0x00000000ffff0000ull) >> 16)

#define XB1(x)  (((x) & 0x00000000ffff0000ull))

#define XB2(x)  (((x) & 0x00000000ffff0000ull) << 16)

#define XB3(x)  (((x) & 0x00000000ffff0000ull) << 32)

#define XC0(x)  (((x) & 0x0000ffff00000000ull) >> 32)

#define XC1(x)  (((x) & 0x0000ffff00000000ull) >> 16)

#define XC2(x)  (((x) & 0x0000ffff00000000ull))

#define XC3(x)  (((x) & 0x0000ffff00000000ull) << 16)

#define XD0(x)  (((x) & 0xffff000000000000ull) >> 48)

#define XD1(x)  (((x) & 0xffff000000000000ull) >> 32)

#define XD2(x)  (((x) & 0xffff000000000000ull) >> 16)

#define XD3(x)  (((x) & 0xffff000000000000ull))

using namespace Pds;

/*
 * shuffle - copy and reorder an array of 16-bit values
 *
 * This routine copies data from an input buffer to an output buffer
 * while also reordering the data.
 * Input ordering: A1 B1 C1 D1 A2 B2 C2 D2 A3 B3 C3 D3...

 * Output ordering: A1 A2 A3... B1 B2 B3... C1 C2 C3... D1 D2 D3...
 *
 * INPUT PARAMETERS:
 *
 *   in - Array of 16-bit elements to be shuffled.
 *        Note the size restrictions below.
 *
 *   nelements - the number of 16-bit elements in the input array.
 *               (NOT the number of bytes, but half that).
 *               Must be a positive multiple of PNCCD::Camex::NumChan * 4.
 *
 * OUPUT PARAMETER:
 *
 *   out - Array of 16-bit elements where result of shuffle is stored.
 *         The caller is responsible for providing a valid pointer
 *         to an output array equal in size but not overlapping the
 *         input array.
 *
 * RETURNS: -1 on error, otherwise 0.
 */

int _shuffle(const void* invoid, void* outvoid, unsigned int nelements)
{
  const uint64_t *in0;
  uint64_t *out0;
  uint64_t *out1;
  uint64_t *out2;
  uint64_t *out3;
  unsigned ii, jj;
  const uint16_t* in = (const uint16_t*)invoid;
  uint16_t* out = (uint16_t*)outvoid;
  unsigned int width = PNCCD::Camex::NumChan * 4;

  if (!in || !out || (nelements < width) || (nelements % width)) {
    /* error */
    return -1;
  }

  // outer loop: shuffle camexes within a line
  for (jj = 0; jj < nelements / width; jj++) {
    in0 = (const uint64_t *)in;
    out0 = (uint64_t *)out;
    out1 = (uint64_t *)(out + (width / 4));
    out2 = (uint64_t *)(out + (width / 2));
    out3 = (uint64_t *)(out + (width * 3 / 4));
    // inner loop: shuffle channels within a camex
    for (ii = 0; ii < width / 4; ii += 4) {
      *out0++ = XA0(in0[ii]) | XA1(in0[ii+1]) | XA2(in0[ii+2]) | XA3(in0[ii+3]);
      *out1++ = XB0(in0[ii]) | XB1(in0[ii+1]) | XB2(in0[ii+2]) | XB3(in0[ii+3]);
      *out2++ = XC0(in0[ii]) | XC1(in0[ii+1]) | XC2(in0[ii+2]) | XC3(in0[ii+3]);
      *out3++ = XD0(in0[ii]) | XD1(in0[ii+1]) | XD2(in0[ii+2]) | XD3(in0[ii+3]);
    }
    in += width;
    out += width;
  }
  /* OK */
  return 0;
}
