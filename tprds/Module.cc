#include "pds/tprds/Module.hh"

#include <stdio.h>

using namespace Pds::TprDS;

void TprBase::dump() const {
  static const unsigned NChan=12;
#define DUMPREG(r) { printf("%16.16s [%p]: %08x\n",#r, &r, r); }
  DUMPREG(irqEnable);
  DUMPREG(irqStatus);
  DUMPREG(partitionAddr);
  DUMPREG(gtxDebug);
  DUMPREG(countReset);
  DUMPREG(dmaFullThr);
  printf("channel0  [%p]\n",&channel[0].control);
  printf("control : ");
  for(unsigned i=0; i<NChan; i++)      printf("%08x ",channel[i].control);
  printf("\nevtCount: ");
  for(unsigned i=0; i<NChan; i++)      printf("%08x ",channel[i].evtCount);
  printf("\nevtSel  : ");
  for(unsigned i=0; i<NChan; i++)      printf("%08x ",channel[i].evtSel);
  printf("\ntestData: ");
  for(unsigned i=0; i<NChan; i++)      printf("%08x ",channel[i].testData);
  printf("\nframeCnt: %08x\n",frameCount);
  printf("pauseCnt: %08x\n",pauseCount);
  printf("ovfloCnt: %08x\n",overflowCount);
  printf("idleCnt : %08x\n",idleCount);
}

void TprBase::setupRate   (unsigned i,
                           unsigned rate,
                           unsigned dataLength) 
{
  channel[i].evtSel   = (1<<30) | (0<<11) | rate; // 
  channel[i].testData = dataLength;
  channel[i].control  = 5;
}

void TprBase::setupDaq    (unsigned i,
                           unsigned partition,
                           unsigned dataLength) 
{
  channel[i].evtSel   = (1<<30) | (3<<11) | partition; // 
  channel[i].testData = dataLength;
  channel[i].control  = 5;
}

void TprBase::disable    (unsigned i)
{
  channel[i].control  = 0;
}

void TprBase::resetCounts()
{
  unsigned b = countReset;
  countReset = b|1;
  usleep(1);
  countReset = b&~1;
}

void TprBase::dmaHistEna (bool v)
{
  unsigned b = countReset;
  if (v)
    b |= (1<<1);
  else
    b &= ~(1<<1);
  countReset = b;
}

