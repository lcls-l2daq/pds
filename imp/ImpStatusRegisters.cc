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
//   <status>
//      <register> <name>version</name> <address>0</address> <lane>0</lane> <vc>2</vc> <size>1</size> </register>
//      <register> <name>regStatus</name> <address>2</address> <lane>0</lane> <vc>2</vc> <size>1</size>
//         <field> <bits>0</bits> <label>LocLinkReady</label> </field>
//         <field> <bits>1</bits> <label>RemLinkReady</label> </field>
//         <field> <bits>2</bits> <label>PibLinkReady</label> </field>
//         <field> <bits>7:4</bits> <label>CntCellError</label> </field>
//         <field> <bits>15:12</bits> <label>CntLinkError</label> </field>
//         <field> <bits>19:16</bits> <label>CntLinkDown</label> </field>
//         <field> <bits>27:24</bits> <label>CntOverFlow</label> </field>
//      </register>
//      <register> <name>txCount</name> <address>3</address> <lane>0</lane> <vc>2</vc> <size>1</size> </register>
//      <register> <name>AdcChipIdReg</name> <address>49153</address> <lane>0</lane> <vc>1</vc> <size>1</size>
//         <field> <bits>7:0</bits><label>AdcChipId</label></field>
//      </register>
//      <register> <name>AdcChipGradeReg</name> <address>49154</address> <lane>0</lane> <vc>1</vc> <size>1</size>
//         <field> <bits>6:4</bits><label>AdcChipGrade</label></field>
//      </register>
//   </status>


using namespace Pds::Imp;

class ImpDestination;
class StatusRegister;
//class StatusLane;

enum Addresses {versionAddr=0, laneStatusAddr=2, txCountAddr=3, chipIdAddr=2};

void ImpStatusRegisters::print() {
//  printf("ImpStatusRegisters: version(0x%x)\n", version);
//  lane.print();
  printf("ImpStatusRegisters: ChipIDReg(0x%x)\n", chipIdReg);
}


void StatusLane::print() {
//  printf("\tLink Status: locLink(%u) remLink(%u) pibLink(%u)\n",
//      locLinkReady, remLinkReady, PibLinkReady);
//  printf("\t\tcellErrorCount(%u) linkErrorCount(%u) linkDownCount(%u) bufferOverflowCount(%u)\n",
//      cellErrorCount, linkErrorCount, linkDownCount, bufferOverflowCount);
}

int ImpStatusRegisters::read() {
  int ret = 0;
  ImpDestination d;
  d.dest(ImpDestination::CommandVC);
//  ret |= pgp->readRegister(&d,versionAddr,0x10,(uint32_t*)&version);
//  ret |= pgp->readRegister(&d,laneStatusAddr,0x11,(uint32_t*)&lane);
//  ret |= pgp->readRegister(&d,txCountAddr,0x12,(uint32_t*)&txCounter);
  ret |= pgp->readRegister(&d,chipIdAddr,0x13,(uint32_t*)&chipIdReg);
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

