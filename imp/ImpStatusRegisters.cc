/*
 * ImpStatusRegisters.cc
 *
 *  Created on: April 12, 2013
 *      Author: jackp
 */

#include <stdio.h>
#include <string.h>
#include "pds/imp/ImpStatusRegisters.hh"
#include "pds/imp/ImpConfigurator.hh"
#include "pds/imp/ImpDestination.hh"
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
//    Register definitions:
//    25 = 0xDEAD0001 usRxReset,
//            0x00000002 is Count Reset,
//            0xDEAD0004 Reset_sft.
//
//    26 = 0x00000001 enable data frames.
//
//    12 =  (31:28) powers okay
//          (27:18) zeroes
//          (17) usRemLinked
//          (16) usLocLinked
//          (15:12) usRxCount
//          (11:8).  UsCellErrCount
//          (7:4).    UsLinkDownCount
//          (3:0).    UsLinkErrCount.
//
//    24 = same as 12, except that the upper 4 bits are zeroes.
//
//    Registers 12 and 24 are read only.
//    Register 12 is returned in the data frames as well as the ranges that are being used.


using namespace Pds::Imp;

class ImpDestination;
class StatusRegister;
//class StatusLane;


void ImpStatusRegisters::print() {
//  printf("ImpStatusRegisters: version(0x%x)\n", version);
  long long unsigned* u = (long long unsigned*) &chipIdRegLow;
  printf("ImpStatusRegisters: version(0x%x) ChipIDReg(0x%llx)\n", version, *u);
  lane.print();
}


void StatusLane::print() {
  printf("\tusLocLinked(%s) usRemLinked(%s) usCellErrCount(%u) usLinkErrCount(%u) usLinkDownCount(%u)\n",
      usRemLinked ? "true" : "false", usLocLinked ? "true" : "false", usCellErrCount, usLinkErrCount, usLinkDownCount);
  printf("\t\tpowersOkay(0x%x)\n", powersOkay);
}

int ImpStatusRegisters::read() {
  int ret = 0;
  ImpDestination d;
  d.dest(ImpDestination::CommandVC);
  if (pgp) {
    ret |= pgp->readRegister(&d,versionAddr,0x10,(uint32_t*)&version);
    ret |= pgp->readRegister(&d,laneStatusAddr,0x11,(uint32_t*)&lane);
    ret |= pgp->readRegister(&d,chipIdAddr,0x12,(uint32_t*)&chipIdRegLow);
    ret |= pgp->readRegister(&d,chipIdAddr+1,0x12,(uint32_t*)&chipIdRegHi);
  } else ret = 1;
  if (ret != 0) {
    printf("\tImp Status Registers encountered error while reading!\n");
  }
  return ret;
}

unsigned ImpStatusRegisters::readVersion(uint32_t* retp) {
//  ImpDestination d;
//  d.dest(ImpDestination::VC2);
//  unsigned ret = pgp->readRegister(&d,versionAddr,0x111,retp);
  return(0);
}

