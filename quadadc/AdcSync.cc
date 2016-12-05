#include "pds/quadadc/AdcSync.hh"

#include <stdio.h>
#include <unistd.h>

using namespace Pds::QuadAdc;

static const unsigned _count = 14;

void AdcSync::set_delay(const uint32_t* delay)
{
  for(unsigned i=0; i<4; i++)
    _delay[2*i] = delay[i];
  
  usleep(1);
  _cmd = (0xf<<16) | (_count<<8);
  usleep(100);
  _cmd = (_count<<8);

  for(unsigned i=0; i<4; i++)
    printf("SyncBit: %u  DelayIn: %u  DelayOut: %u\n",
           i, delay[i], _delay[2*i+1]);
}

void AdcSync::start_training()
{
  _cmd = (_count<<8) | 1;
  printf("AdcSync set %x\n",_cmd);
}

void AdcSync::stop_training()
{
  _cmd = (_count<<8) | 0;
  printf("AdcSync set %x\n",_cmd);
}

void AdcSync::dump_status() const
{
  unsigned stats[4];
  for(unsigned j=0; j<4; j++) {
    stats[j] = 0;
    for(unsigned i=0; i<8; i++) {
      usleep(10);
      _select = (j<<16) | i;
      usleep(10);
      unsigned v = _stats;
      stats[j] += v;
    }
  }
  printf("---sync stats---\n");
  for(unsigned i=0; i<8; i++) {
    for(unsigned j=0; j<4; j++) {
      usleep(10);
      _select = (j<<16) | i;
      usleep(10);
      unsigned v = _stats;
      if (stats[j])
        printf(" %04x [%02u%%]", v, 100*v/stats[j]);
      else
        printf(" %04x [---]", v);
    }
    printf("\n");
  }
  for(unsigned j=0; j<4; j++)
    printf(" %04x      ", stats[j]);
  printf("\n");
}
