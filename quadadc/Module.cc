#include "pds/quadadc/Module.hh"

#include <stdio.h>

using namespace Pds::QuadAdc;

void DmaCore::init(unsigned maxDmaSize)
{
  //  Need to disable rx to reset dma size
  rxEnable = 0;
  if (maxDmaSize)
    maxRxSize = maxDmaSize;
  else
    maxRxSize = (1<<31);
  rxEnable = 1;
  
  fifoValid  = 510;
  irqHoldoff = 0;
}

void DmaCore::dump() const 
{
#define PR(r) printf("%9.9s: %08x\n",#r, r)

  printf("DmaCore @ %p\n",this);
  PR(rxEnable);
  PR(txEnable);
  PR(fifoClear);
  PR(irqEnable);
  PR(fifoValid);
  PR(maxRxSize);
  PR(mode);
  PR(irqStatus);
  PR(irqRequests);
  PR(irqAcks);

#undef PR
}

void PhyCore::dump() const
{
  uint32_t* p = (uint32_t*)this;
  for(unsigned i=0x130/4; i<0x238/4; i++)
    printf("%08x%c", p[i], (i%8)==7?'\n':' ');
  printf("\n");
}

void RingBuffer::enable(bool v)
{
  unsigned r = _csr;
  if (v)
    r |= (1<<31);
  else
    r &= ~(1<<31);
  _csr = r;
}

void RingBuffer::clear()
{
  unsigned r = _csr;
  _csr = r | (1<<30);
  _csr = r &~(1<<30);
}

void RingBuffer::dump()
{
  unsigned len = _csr & 0xfffff;
#if 0
  //  These memory accesses translate to more than just 32-bit reads
  uint32_t* buff = new uint32_t[len];
  for(unsigned i=0; i<len; i++)
    buff[i] = _dump[i];
  for(unsigned i=0; i<len; i++)
    printf("%08x%c", buff[i], (i&0x7)==0x7 ? '\n':' ');
  delete[] buff;
#else
  for(unsigned i=0; i<len; i++)
    printf("%08x%c", _dump[i], (i&0x7)==0x7 ? '\n':' ');
#endif
}

void QABase::init()
{
  unsigned v = csr;
  csr = v & ~(1<<31);
}

void QABase::start()
{
  unsigned v = csr;
  csr = v | (1<<31);

  irqEnable = 1;
}

void QABase::stop()
{
  unsigned v = csr;
  csr = v & ~(1<<31);
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
  usleep(10);
  csr = v | (1<<31);
}

void QABase::setupRate(unsigned rate,
                       unsigned length)
{
  acqSelect = (1<<30) | (0<<11) | rate;
  samples   = length;
  prescale  = 1;
  unsigned v = csr & ~(1<<0);
  csr = v | (1<<0);
  usleep(10);
  csr = v | (1<<31);
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

#undef PR
}
