/*
 * CspadQuadRegisters.cc
 *
 *  Created on: Jan 19, 2011
 *      Author: jackp
 */

#include "pds/cspad/CspadConfigurator.hh"
#include "pds/cspad/CspadQuadRegisters.hh"
#include "pds/cspad/CspadLinkCounters.hh"
#include "pds/cspad/CspadRegisterAddrs.hh"
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "PgpCardMod.h"
#include <stdio.h>
#include <unistd.h>
#include <new>

using namespace Pds::CsPad;

class LinkCounters;
class CspadDirectRegisterReader;

CspadQuadRegisters::CspadQuadRegisters() : version(0xdeadbeef) {}

CspadQuadRegisters::~CspadQuadRegisters() {}

//  acqTimer.setAddr       (0x300004);
//  ampIdle.setAddr        (0x300005);
//  injTotal.setAddr       (0x300006);
//  acqCount.setAddr       (0x300007);
//  cmdCount.setAddr       (0x300008);
//  shiftTest.setAddr      (0x400000);     !!!!! removed
//  rowColShiftPer.setAddr (0x400001);
//  version.setAddr        (0x500000);
//  cntRxCellError.setAddr (0x500001);
//  cntRxLinkDown.setAddr  (0x500002);
//  cntRxLinkError.setAddr (0x500003);
//  reset.setAddr          (0x500004);
//  countReset.setAddr     (0x500005);
//  readTimer.setAddr      (0x500006);
//  bufferError.setAddr    (0x500007);
//  tempA.setAddr          (0x600000);
//  tempB.setAddr          (0x600001);
//  tempC.setAddr          (0x600002);
//  tempD.setAddr          (0x600003);

CspadConfigurator* CspadQuadRegisters::cfgtr = 0;

unsigned CspadQuadRegisters::_foo[][2] = {
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

CspadRegisterAddrs* CspadQuadRegisters::_regLoc = (CspadRegisterAddrs*)_foo;
CspadDestination* CspadQuadRegisters::_d = new Pds::CsPad::CspadDestination::CspadDestination();

unsigned CspadQuadRegisters::read(unsigned index) {
  _d->dest(index);
  uint32_t* up = (uint32_t*) this;
  unsigned i=0;
  int ret = 0;
  do {
    ret = cfgtr->pgp()->readRegister(_d, _regLoc[i].addr, 0xf400+(index<<4)+i, up+i);
  } while ( ret == 0 && _regLoc[i++].readFlag < 2);
  if (ret != 0) printf("CspadQuadRegisters::read failed on %u\n", i==0 ? 0 : i-1);
  return ret;
}

void CspadQuadRegisters::print() {
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

void CspadQuadRegisters::printTemps() {
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
