/*
 * Cspad2x2LinkRegisters.hh
 *
 *  Created on: Jan 9, 2012
 *      Author: jackp
 */

#ifndef CSPAD2x2LINKREGISTERS_HH_
#define CSPAD2x2LINKREGISTERS_HH_

//#include <stdint.h>

#include "pds/cspad2x2/Cspad2x2LinkCounters.hh"

namespace Pds {

  namespace CsPad2x2 {

  class Cspad2x2LinkRegisters {
    public:
      Cspad2x2LinkRegisters() : status(0), rxCount(0) {};
      ~Cspad2x2LinkRegisters() {};

      void print();

    public:
      unsigned remLink() { return status&2 ? 1 : 0;}
      unsigned locLink() { return status&1 ? 1 : 0;}

    public:
      enum {LocLinkMask=1, RemLinkMask=2};
      uint32_t      status;
      uint32_t      rxCount;
      Cspad2x2LinkCounters  l;
  };

}

}

#endif /* CSPADLINKREGISTERS_HH_ */
