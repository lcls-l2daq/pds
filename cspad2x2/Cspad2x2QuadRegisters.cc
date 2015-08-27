/*
 * Cspad2x2QuadRegisters.cc
 *
 *  Created on: Jan 9, 2012
 *      Author: jackp
 */

#include "pds/cspad2x2/Cspad2x2Configurator.hh"
#include "pds/cspad2x2/Cspad2x2QuadRegisters.hh"
#include "pds/cspad2x2/Cspad2x2LinkCounters.hh"
#include "pds/cspad2x2/Cspad2x2RegisterAddrs.hh"
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "PgpCardMod.h"
#include <stdio.h>
#include <unistd.h>
#include <new>

using namespace Pds::CsPad2x2;

class LinkCounters;
class Cspad2x2DirectRegisterReader;

Cspad2x2QuadRegisters::Cspad2x2QuadRegisters() : version(0xdeadbeef) {}

Cspad2x2QuadRegisters::~Cspad2x2QuadRegisters() {}

//  acqTimer       (0x300004);
//  ampIdle        (0x300005);
//  injTotal       (0x300006);
//  acqCount       (0x300007);
//  cmdCount       (0x300008);
//  shiftTest      (0x400000);    !!! commented out
//  rowColShiftPer (0x400001);
//  version        (0x500000);
//  cntRxCellError (0x500001);
//  cntRxLinkDown  (0x500002);
//  cntRxLinkError (0x500003);
//  reset          (0x500004);
//  countReset     (0x500005);
//  readTimer      (0x500006);
//  bufferError    (0x500007);
//  tempA          (0x600000);
//  tempB          (0x600001);
//  tempC          (0x600002);
//  tempD          (0x600003);

// 0x700006 : dacValue,   16-bit Peltier temperature ?????

Cspad2x2Configurator* Cspad2x2QuadRegisters::cfgtr = 0;

unsigned Cspad2x2QuadRegisters::_foo[][2] = {
    {0x300004, 1},   //  acqTimer
    {0x300007, 1},   //  acqCount
    {0x300008, 1},   //  cmdCount
//    {0x400000, 1},   //  shiftTest
    {0x500000, 1},   //  version
    {0x500001, 1},   //  RxCounters cell errors
    {0x500002, 1},   //     ..      link down
    {0x500003, 1},   //     ..      link errrors
    {0x500006, 1},   //  readTimer
    {0x500007, 1},   //  bufferError
    {0x600000, 1},   //  temperatures[4]
    {0x600001, 1},   //   ..
    {0x600002, 1},   //   ..
    {0x600003, 2}    //   ..  read flag > 1 marks the last read
};

Cspad2x2RegisterAddrs* Cspad2x2QuadRegisters::_regLoc = (Cspad2x2RegisterAddrs*)_foo;
Cspad2x2Destination* Cspad2x2QuadRegisters::_d = new Pds::CsPad2x2::Cspad2x2Destination();

unsigned Cspad2x2QuadRegisters::read() {
  _d->dest(Cspad2x2Destination::Q0);
  uint32_t* up = (uint32_t*) this;
  unsigned i=0;
  int ret = 0;
  do {
    ret = cfgtr->pgp()->readRegister(_d, _regLoc[i].addr, 0xf400+i, up+i);
  } while ( ret == 0 && _regLoc[i++].readFlag < 2);
  if (ret != 0) printf("Cspad2x2QuadRegisters::read failed on index %u\n", i);
  return ret;
}

void Cspad2x2QuadRegisters::print() {
  if (version!=0xdeadbeef) {
    printf("\tversion(0x%x)\n", (unsigned)version);
    printf("\tacqTimer(%u)(%uns)\n", (unsigned)acqTimer, (unsigned)acqTimer<<3);
    printf("\treadTimer(%u)(%uns)\n", (unsigned)readTimer, (unsigned)readTimer<<5);
    printf("\tacqCount(%u)\n", (unsigned)acqCount);
    printf("\tcmdCount(%u)\n", (unsigned)cmdCount);
//    printf("\tshiftTest(0x%x)\n", (unsigned)shiftTest);
    printf("\tRx "); RxCounters.print(); printf("\n");
    printf("\tbufferError(%u)\n", (unsigned)bufferError);
    printf("\ttemperatures(");
    for (int i=0; i<numbTemps; i++) {printf("%u%s",
        (unsigned)temperatures[i],
        i<(numbTemps-1) ? ", " : ")\n");
    }
    version = 0xdeadbeef;
  } else printf("\thave not been read.\n");
}

void Cspad2x2QuadRegisters::printTemps() {
  if (version!=0xdeadbeef) {
    printf("\ttemperatures( ");
    for (int i=0; i<numbTemps; i++) {
      //      if (temperatures[i] && (temperatures[i] != 4095))
      printf("%4u ", (unsigned)temperatures[i]);
    }
    printf(")\n");
    version = 0xdeadbeef;
  } else {
    printf("\thave not been read\n");
  }
}
