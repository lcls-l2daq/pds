/*
 * CspadConcentratorRegisters.cc
 *
 *  Created on: Jan 21, 2011
 *      Author: jackp
 */
#include "pds/cspad/CspadConcentratorRegisters.hh"
#include "pds/cspad/CspadLinkCounters.hh"
#include "pds/cspad/CspadRegisterAddrs.hh"
#include "pds/cspad/CspadConfigurator.hh"
#include "pds/pgp/RegisterSlaveImportFrame.hh"
#include "pds/pgp/RegisterSlaveExportFrame.hh"
#include "pds/pgp/Destination.hh"
#include "pds/cspad/CspadConfigurator.hh"
#include "pds/cspad/CspadDestination.hh"
#include "pdsdata/cspad/Detector.hh"
#include <time.h>

//   version.setAddr          (0x000000);
//   reset.setAddr            (0x000001);
//   countReset.setAddr       (0x000002);
//   ds0RxReset.setAddr       (0x000003);
//   ds1RxReset.setAddr       (0x000004);
//   ds2RxReset.setAddr       (0x000005);
//   ds3RxReset.setAddr       (0x000006);
//   us0RxReset.setAddr       (0x000007);
//   us1RxReset.setAddr       (0x000008);
//   cmdSeqCountClear.setAddr (0x000009);
//   cmdSeqCount.setAddr      (0x00000A);
//   cycleTime.setAddr        (0x00000B);
//   ds0Status.setAddr        (0x00000C);
//   ds0RxCount.setAddr       (0x00000D);
//   ds0CellErrorCnt.setAddr  (0x00000E);
//   ds0LinkDownCnt.setAddr   (0x00000F);
//   ds0LinkErrorCnt.setAddr  (0x000010);
//   ds1Status.setAddr        (0x000011);
//   ds1RxCount.setAddr       (0x000012);
//   ds1CellErrorCnt.setAddr  (0x000013);
//   ds1LinkDownCnt.setAddr   (0x000014);
//   ds1LinkErrorCnt.setAddr  (0x000015);
//   ds2Status.setAddr        (0x000016);
//   ds2RxCount.setAddr       (0x000017);
//   ds2CellErrorCnt.setAddr  (0x000018);
//   ds2LinkDownCnt.setAddr   (0x000019);
//   ds2LinkErrorCnt.setAddr  (0x00001A);
//   ds3Status.setAddr        (0x00001B);
//   ds3RxCount.setAddr       (0x00001C);
//   ds3CellErrorCnt.setAddr  (0x00001D);
//   ds3LinkDownCnt.setAddr   (0x00001E);
//   ds3LinkErrorCnt.setAddr  (0x00001F);
//   us0Status.setAddr        (0x000020);
//   us0RxCount.setAddr       (0x000021);
//   us0CellErrorCnt.setAddr  (0x000022);
//   us0LinkDownCnt.setAddr   (0x000023);
//   us0LinkErrorCnt.setAddr  (0x000024);
//   us1Status.setAddr        (0x000025);
//   us1RxCount.setAddr       (0x000026);
//   us1CellErrorCnt.setAddr  (0x000027);
//   us1LinkDownCnt.setAddr   (0x000028);
//   us1LinkErrorCnt.setAddr  (0x000029);
//   runMode.setAddr          (0x00002A);
//   resetQuads.setAddr       (0x00002C);
//   dsFlowCntrl.setAddr      (0x00002D);
//   usFlowCntrl.setAddr      (0x00002E);
//   ds00DataCount.setAddr    (0x00002F);
//   ds01DataCount.setAddr    (0x000030);
//   us0DataCount.setAddr     (0x000031);
//   ds10DataCount.setAddr    (0x000032);
//   ds11DataCount.setAddr    (0x000033);
//   us1DataCount.setAddr     (0x000034);
//   evrErrors.setAddr        (0x000035);
//   evrEnable.setAddr        (0x000100);
//   evrOpCode.setAddr        (0x000146);
//   evrDelay.setAddr         (0x000101);
//   evrWidth.setAddr         (0x000102);
using namespace Pds::CsPad;

class LinkCounters;
class CspadDirectRegisterReader;

CspadConcentratorRegisters::CspadConcentratorRegisters() {}

CspadConcentratorRegisters::~CspadConcentratorRegisters() {}

// register address in FE, read flag
unsigned CspadConcentratorRegisters::foo[][2] = {
    // readFlag of >0 means read it.
    {0x000000, 1},  //   version
    {0x00000a, 1},  //   cmdSeqCount
    {0x00000b, 1},  //   cycleTime
    {0x00000c, 1},  //   ds0
    {0x00000d, 1},
    {0x00000e, 1},
    {0x00000f, 1},
    {0x000010, 1},
    {0x000011, 1},  //   ds1
    {0x000012, 1},
    {0x000013, 1},
    {0x000014, 1},
    {0x000015, 1},
    {0x000016, 1},  //   ds2
    {0x000017, 1},
    {0x000018, 1},
    {0x000019, 1},
    {0x00001a, 1},
    {0x00001b, 1},  //   ds3
    {0x00001c, 1},
    {0x00001d, 1},
    {0x00001e, 1},
    {0x00001f, 1},
    {0x000020, 1},  //   us0
    {0x000021, 1},
    {0x000022, 1},
    {0x000023, 1},
    {0x000024, 1},
    {0x000025, 1},  //   us1
    {0x000026, 1},
    {0x000027, 1},
    {0x000028, 1},
    {0x000029, 1},
    {0x00002d, 1},  //   dsFlowCntrl
    {0x00002e, 1},  //   usFlowCntrl
    {0x00002f, 1},  //   ds00DataCount
    {0x000030, 1},  //   ds01DataCount
    {0x000031, 1},  //   us0DataCount
    {0x000032, 1},  //   ds10DataCount
    {0x000033, 1},  //   ds11DataCount
    {0x000034, 1},  //   us1DataCount
    {0x000035, 2}   //   evrErrors
    // readFlag > 1 marks the last read
};

CspadRegisterAddrs* CspadConcentratorRegisters::_regLoc = (CspadRegisterAddrs*) CspadConcentratorRegisters::foo;
CspadConfigurator* CspadConcentratorRegisters::cfgtr = 0;
bool CspadConcentratorRegisters::hasBeenRead = false;
CspadDestination* CspadConcentratorRegisters::dest =
    new CspadDestination::CspadDestination();

unsigned CspadConcentratorRegisters::read() {
  timespec      sleepTime;
  sleepTime.tv_sec = 0;
  sleepTime.tv_nsec = 25000000; // 25ms
  uint32_t* up = (uint32_t*) this;
  unsigned i=0;
  int ret = 0;
  dest->dest(CspadDestination::CR);
  cfgtr->pgp()->writeRegister(dest, Pds::CsPad::CspadConfigurator::RunModeAddr, Pds::CsPad::NoRunning);
  nanosleep(&sleepTime, 0); nanosleep(&sleepTime, 0);
  do {
    ret = cfgtr->pgp()->readRegister(dest, _regLoc[i].addr, 0xfc00+i, up+i);
  } while ( ret == 0 && _regLoc[i++].readFlag < 2);
  if (ret != 0) {
    printf("CspadQuadRegisters::read failed on %u\n", i==0 ? 0 : i-1);
  } else {
    hasBeenRead = true;
  }
  cfgtr->pgp()->writeRegister(dest, Pds::CsPad::CspadConfigurator::RunModeAddr, Pds::CsPad::RunButDrop);
  return ret;
}

void CspadConcentratorRegisters::print() {
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
