/*
 * CspadLinkRegisters.hh
 *
 *  Created on: Jan 21, 2011
 *      Author: jackp
 */

#ifndef CSPADLINKREGISTERS_HH_
#define CSPADLINKREGISTERS_HH_

//#include <stdint.h>

#include "pds/csPad/CspadLinkCounters.hh"

namespace Pds {

  namespace CsPad {

  class CspadLinkRegisters {
    public:
      CspadLinkRegisters() : status(0), rxCount(0) {};
      ~CspadLinkRegisters() {};

      void print();

    public:
      unsigned remLink() { return status&2 ? 1 : 0;}
      unsigned locLink() { return status&1 ? 1 : 0;}

    public:
      enum {LocLinkMask=1, RemLinkMask=2};
      uint32_t      status;
      uint32_t      rxCount;
      CspadLinkCounters  l;
  };

}

}

#endif /* CSPADLINKREGISTERS_HH_ */
