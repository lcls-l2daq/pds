/*
 * XampsInternalRegisters.cc
 *
 *  Created on: Jan 21, 2011
 *      Author: jackp
 */

#include <stdio.h>
#include "pds/xamps/XampsInternalRegisters.hh"

namespace Pds {
  namespace Xamps {

    void InternalLane::print() {
      printf("\t\tlocLink(%u) remLink(%u) rnlLink(%u)\n",
          locLinkReady, remLinkReady, linked);
      printf("\t\tcellErrorCount(%u) linkErrorCount(%u) linkDownCount(%u)\n",
          cellErrorCount, linkErrorCount, linkDownCount);
      printf("\t\tbufferOverflowCount(%u) txCounter(%u)\n",
          bufferOverflowCount, txCounter);
    }

    void XampsInternalRegisters::print() {
      printf("XAMPS Internal Registers: version(0x%x), scratchPad(0x%x)\n",
          version, scratchPad);
      for (unsigned i=0; i<NumberOfLanes; i++) {
        printf("\tLane %u:\n", i);
        lanes[i].print();
      }
    }
  }
}
