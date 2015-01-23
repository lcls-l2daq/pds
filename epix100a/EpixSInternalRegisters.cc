/*
 * EpixSStatusRegisters.cc
 *
 *  Created on: 2014.7.31
 *      Author: jackp
 */

#include <stdio.h>
#include "pds/epixS/EpixSStatusRegisters.hh"

namespace Pds {
  namespace EpixS {

    void StatusLane::print() {
      printf("\t\tlocLink(%u) remLink(%u) pibLink(%u)\n",
          locLinkReady, remLinkReady, PibLinkReady);
      printf("\t\tcellErrorCount(%u) linkErrorCount(%u) linkDownCount(%u)\n",
          cellErrorCount, linkErrorCount, linkDownCount);
      printf("\t\tbufferOverflowCount(%u) txCounter(%u)\n",
          bufferOverflowCount, txCounter);
    }

    void EpixSStatusRegisters::print() {
      printf("XAMPS Status Registers: version(0x%x), scratchPad(0x%x)\n",
          version, scratchPad);
      for (unsigned i=0; i<NumberOfLanes; i++) {
        printf("\tLane %u:\n", i);
        lanes[i].print();
      }
    }
  }
}
