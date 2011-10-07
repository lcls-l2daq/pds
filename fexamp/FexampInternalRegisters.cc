/*
 * FexampStatusRegisters.cc
 *
 *  Created on: Jan 21, 2011
 *      Author: jackp
 */

#include <stdio.h>
#include "pds/fexamp/FexampStatusRegisters.hh"

namespace Pds {
  namespace Fexamp {

    void StatusLane::print() {
      printf("\t\tlocLink(%u) remLink(%u) pibLink(%u)\n",
          locLinkReady, remLinkReady, PibLinkReady);
      printf("\t\tcellErrorCount(%u) linkErrorCount(%u) linkDownCount(%u)\n",
          cellErrorCount, linkErrorCount, linkDownCount);
      printf("\t\tbufferOverflowCount(%u) txCounter(%u)\n",
          bufferOverflowCount, txCounter);
    }

    void FexampStatusRegisters::print() {
      printf("XAMPS Status Registers: version(0x%x), scratchPad(0x%x)\n",
          version, scratchPad);
      for (unsigned i=0; i<NumberOfLanes; i++) {
        printf("\tLane %u:\n", i);
        lanes[i].print();
      }
    }
  }
}
