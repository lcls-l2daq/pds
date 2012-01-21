/*
 * Cspad2x2ConcentratorRegisters.cc
 *
 *  Created on: Jan 9, 2012
 *      Author: jackp
 */
#include "pds/cspad2x2/Cspad2x2ConcentratorRegisters.hh"
#include "pds/cspad2x2/Cspad2x2LinkCounters.hh"
#include "pds/cspad2x2/Cspad2x2RegisterAddrs.hh"
#include "pds/cspad2x2/Cspad2x2Configurator.hh"
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/pgp/Destination.hh"
#include "pds/cspad2x2/Cspad2x2Configurator.hh"
#include "pds/cspad2x2/Cspad2x2Destination.hh"
#include "pdsdata/cspad2x2/Detector.hh"
#include <time.h>

//   version.setAddr          (0x000000);
//   reset.setAddr            (0x000001);
//   countReset.setAddr       (0x000002);
//   ds0RxReset.setAddr       (0x000003);
//   us0RxReset.setAddr       (0x000007);
//   cmdSeqCountClear.setAddr (0x000009);
//   cmdSeqCount.setAddr      (0x00000A);
//   cycleTime.setAddr        (0x00000B);
//   ds0Status.setAddr        (0x00000C);
//   ds0RxCount.setAddr       (0x00000D);
//   ds0CellErrorCnt.setAddr  (0x00000E);
//   ds0LinkDownCnt.setAddr   (0x00000F);
//   ds0LinkErrorCnt.setAddr  (0x000010);
//   us0Status.setAddr        (0x000020);
//   us0RxCount.setAddr       (0x000021);
//   us0CellErrorCnt.setAddr  (0x000022);
//   us0LinkDownCnt.setAddr   (0x000023);
//   us0LinkErrorCnt.setAddr  (0x000024);
//   runMode.setAddr          (0x00002A);
//   resetQuads.setAddr       (0x00002C);
//   dsFlowCntrl.setAddr      (0x00002D);
//   usFlowCntrl.setAddr      (0x00002E);
//   ds00DataCount.setAddr    (0x00002F);
//   us0DataCount.setAddr     (0x000031);
//   us1DataCount.setAddr     (0x000034);
using namespace Pds::CsPad2x2;

class LinkCounters;
class Cspad2x2DirectRegisterReader;

Cspad2x2ConcentratorRegisters::Cspad2x2ConcentratorRegisters() {}

Cspad2x2ConcentratorRegisters::~Cspad2x2ConcentratorRegisters() {}

// register address in FE, read flag
unsigned Cspad2x2ConcentratorRegisters::foo[][2] = {
    // readFlag of >0 means read it.
    {0x000000, 1},  //   version
    {0x00000a, 1},  //   cmdSeqCount
    {0x00000b, 1},  //   cycleTime
    {0x00000c, 1},  //   ds0
    {0x00000d, 1},
    {0x00000e, 1},
    {0x00000f, 1},
    {0x000010, 1},
    {0x000020, 1},  //   us0
    {0x000021, 1},
    {0x000022, 1},
    {0x000023, 1},
    {0x000024, 1},
    {0x00002d, 1},  //   dsFlowCntrl
    {0x00002e, 1},  //   usFlowCntrl
    {0x00002f, 1},  //   ds00DataCount
    {0x000031, 1},  //   us0DataCount
    {0x000035, 2}   //   evrErrors
    // readFlag > 1 marks the last read
};

Cspad2x2RegisterAddrs* Cspad2x2ConcentratorRegisters::_regLoc = (Cspad2x2RegisterAddrs*) Cspad2x2ConcentratorRegisters::foo;
Cspad2x2Configurator* Cspad2x2ConcentratorRegisters::cfgtr = 0;
bool Cspad2x2ConcentratorRegisters::hasBeenRead = false;
Cspad2x2Destination* Cspad2x2ConcentratorRegisters::dest =
    new Cspad2x2Destination::Cspad2x2Destination();

unsigned Cspad2x2ConcentratorRegisters::read() {
  timespec      sleepTime;
  sleepTime.tv_sec = 0;
  sleepTime.tv_nsec = 25000000; // 25ms
  uint32_t* up = (uint32_t*) this;
  unsigned i=0;
  int ret = 0;
  dest->dest(Cspad2x2Destination::CR);
  cfgtr->pgp()->writeRegister(dest, Pds::CsPad2x2::Cspad2x2Configurator::RunModeAddr, Pds::CsPad2x2::NoRunning);
  nanosleep(&sleepTime, 0); nanosleep(&sleepTime, 0);
  do {
    ret = cfgtr->pgp()->readRegister(dest, _regLoc[i].addr, 0xfc00+i, up+i);
  } while ( ret == 0 && _regLoc[i++].readFlag < 2);
  if (ret != 0) {
    printf("Cspad2x2QuadRegisters::read failed on %u\n", i==0 ? 0 : i-1);
  } else {
    hasBeenRead = true;
  }
  cfgtr->pgp()->writeRegister(dest, Pds::CsPad2x2::Cspad2x2Configurator::RunModeAddr, Pds::CsPad2x2::RunButDrop);
  return ret;
}

void Cspad2x2ConcentratorRegisters::print() {
  printf("Concentrator Registers:\n");
  if (hasBeenRead) {
    printf("\tversion(0x%x)\n", (unsigned)version);
    printf("\tcmdSeqCount(%u)\n", (unsigned)cmdSeqCount);
    printf("\tcycleTime(%u)(%u.%uns)\n\n", (unsigned)cycleTime,
        (unsigned)(cycleTime<<6)/10, (unsigned)(cycleTime<<6)%10);
    for (unsigned i=0; i<numDSLinks; i++) {
      printf("\tLink DS%u: ", i);
      ds[i].print();
      printf("\n");
    }
    printf("\tdsFlowControl(0x%x)\n\tds00DataCount(%u), ds01DataCount(%u), ds10DataCount(%u), ds11DataCount(%u)\n\n",
        (unsigned) dsFlowCntrl, (unsigned)ds00DataCount, (unsigned)ds01DataCount, (unsigned)ds10DataCount, (unsigned)ds11DataCount);
    for (unsigned i=0; i<numUSLinks; i++) {
      printf("\tLink US%u: ", i);
      us[i].print();
      printf("\n");
    }
    printf("\tusFlowControl(0x%x), us0DataCount(%u), us1DataCount(%u)\n",
        (unsigned) usFlowCntrl, (unsigned)us0DataCount, (unsigned)us1DataCount);
    printf("\tevrErrors(%u)\n", (unsigned)evrErrors);
    hasBeenRead = false;
  } else printf("\thave not been read.\n");
}
