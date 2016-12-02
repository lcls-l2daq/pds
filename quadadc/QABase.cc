#include "pds/quadadc/QABase.hh"

#include <unistd.h>
#include <stdio.h>

using namespace Pds::QuadAdc;

void QABase::init()
{
  unsigned v = csr;
  csr = v & ~(1<<31);
}

void QABase::start()
{
  unsigned v = csr;
  //  csr = v | (1<<31) | (1<<1);
  csr = v | (1<<31);

  irqEnable = 1;
}

void QABase::stop()
{
  unsigned v = csr;
  csr = v & ~(1<<31) & ~(1<<1);
}

void QABase::resetCounts()
{
  unsigned v = csr;
  csr = v | (1<<0);
  usleep(10);
  csr = v & ~(1<<0);
}

void QABase::setChannels(unsigned ch)
{
  unsigned v = control;
  v &= ~0xf;
  v |= (ch&0xf);
  control = v;
}

void QABase::setMode (Interleave q)
{
  unsigned v = control;
  if (q) v |=  (1<<8);
  else   v &= ~(1<<8);
  control = v;
}

void QABase::setupDaq(unsigned partition,
                      unsigned length)
{
  acqSelect = (1<<30) | (3<<11) | partition;
  samples   = length;
  prescale  = 1;
  unsigned v = csr & ~(1<<0);
  csr = v | (1<<0);
}

void QABase::setupLCLS(unsigned rate,
                       unsigned length)
{
  acqSelect = rate&0xff;
  samples   = length;
  prescale  = 1;
  unsigned v = csr & ~(1<<0);
  csr = v | (1<<0);
}

void QABase::setupLCLSII(unsigned rate,
                         unsigned length)
{
  acqSelect = (1<<30) | (0<<11) | rate;
  samples   = length;
  prescale  = 1;
  unsigned v = csr & ~(1<<0);
  csr = v | (1<<0);
}

void QABase::enableDmaTest(bool enable)
{
  unsigned v = csr;
  if (enable)
    v |= (1<<2);
  else
    v &= ~(1<<2);
  csr = v;
}

void QABase::resetClock(bool r)
{
  unsigned v = csr;
  if (r) 
    v |= (1<<3);
  else
    v &= ~(1<<3);
  csr = v;

  if (!r) {
    // check for locked bit
    for(unsigned i=0; i<5; i++) {
      if (clockLocked()) {
        printf("clock locked\n");
        return;
      }
      usleep(10000);
    }
    printf("clock not locked\n");
  }
}

bool QABase::clockLocked() const
{
  unsigned v = adcSync;
  return v&(1<<31);
}

void QABase::dump() const
{
#define PR(r) printf("%9.9s: %08x\n",#r, r)

  PR(irqEnable);
  PR(irqStatus);
  PR(partitionAddr);
  PR(dmaFullThr);
  PR(csr);
  PR(acqSelect);
  PR(control);
  PR(samples);
  PR(prescale);
  PR(offset);
  PR(countAcquire);
  PR(countEnable);
  PR(countInhibit);
  PR(dmaFullQ);
  PR(adcSync);

#undef PR
}
