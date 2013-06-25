/*
 * pnCCDConfigurator.cc
 *
 *  Created on: May 30, 2013
 *      Author: jackp
 */

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <mqueue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "pds/pgp/Configurator.hh"
#include "pds/pnccd/pnCCDConfigurator.hh"
#include "pds/config/pnCCDConfigType.hh"

using namespace Pds::pnCCD;

class pnCCDDestination;

enum {pnCCDMagicWord=0x1c000300,pnCCDFrameSize=131075}; // ((1<<19)/4) + 4 - 1

pnCCDConfigurator::pnCCDConfigurator(int f, unsigned d) :
                           Pds::Pgp::Configurator(f, d),
                           _rhisto(0) {
  printf("pnCCDConfigurator constructor\n");
  //    printf("\tlocations _pool(%p), _config(%p)\n", _pool, &_config);
  //    _rhisto = (unsigned*) calloc(1000, 4);
  //    _lhisto = (LoopHisto*) calloc(4*10000, 4);
}

pnCCDConfigurator::~pnCCDConfigurator() {}

void pnCCDConfigurator::runTimeConfigName(char* name) {
  if (name) strcpy(_runTimeConfigFileName, name);
  printf("pnCCDConfigurator::runTimeConfigName(%s)\n", name);
}

void pnCCDConfigurator::print() {}

void pnCCDConfigurator::printMe() {
  printf("Configurator: ");
  for (unsigned i=0; i<sizeof(*this)/sizeof(unsigned); i++) printf("\n\t%08x", ((unsigned*)this)[i]);
  printf("\n");
}

bool pnCCDConfigurator::_flush(unsigned index=0) {
//  enum {numberOfTries=5};
//  unsigned version = 0;
//  unsigned failCount = 0;
  bool ret = false;
//  printf("\n\t--flush-%u-", index);
//  while ((failCount++<numberOfTries) && (Failure == _statRegs.readVersion(&version))) {
//    printf("%s(%u)-", _d.name(), failCount);
//  }
//  if (failCount<numberOfTries) {
//    printf("%s version(0x%x)\n\t", _d.name(), version);
//  }
//  else {
    ret = true;
//    printf("\n\t");
//  }
  return ret;
}

unsigned pnCCDConfigurator::configure( pnCCDConfigType* c, unsigned mask) {
  int writeReturn = 0;
  _config = c;
  timespec      start, end, sleepTime, shortSleepTime;
  sleepTime.tv_sec = 0;
  sleepTime.tv_nsec = 25000000; // 25ms
  shortSleepTime.tv_sec = 0;
  shortSleepTime.tv_nsec = 5000000;  // 5ms (10 ms is shortest sleep on some computers
  bool printFlag = !(mask & 0x2000);
  mask = ~mask;
  unsigned ret = 0;
  clock_gettime(CLOCK_REALTIME, &start);
  PgpCardTx           p;
  p.model = sizeof(&p);
  p.cmd   = IOCTL_Write_Scratch;
  //  payload  divided by bytes per qwords  - 1 as per Ryan
  p.data  = (unsigned*)(((c->payloadSizePerLink())/4) - 1);
  if (printFlag) printf("pnCCD Config fd(%d) addr(%p) model(%u) data(%p)\n", fd(), &p, p.model, p.data);
  writeReturn = write(fd(), &p, sizeof(p));
  if (printFlag) {
    clock_gettime(CLOCK_REALTIME, &end);
    uint64_t diff = timeDiff(&end, &start) + 50000LL;
    printf("\tret(%u) size(%u) - done, %s \n", ret, sizeof(p), ret ? "FAILED" : "SUCCEEDED");
    printf(" it took %lld.%lld milliseconds with mask 0x%x\n", diff/1000000LL, diff%1000000LL, mask&0x1f);
  }
  dumpFrontEnd();
  return ret;
}

void pnCCDConfigurator::dumpFrontEnd() {
  dumpPgpCard();
}
