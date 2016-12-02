#include "pds/quadadc/AdcSync.hh"

#include <stdio.h>
#include <unistd.h>

using namespace Pds::QuadAdc;

void AdcSync::set_delay(const uint32_t* delay)
{
  for(unsigned i=0; i<4; i++)
    _delay[2*i] = delay[i];
  
  usleep(1);
  _cmd = 0xf<<16;
  usleep(100);
  _cmd = 0;

  for(unsigned i=0; i<4; i++)
    printf("SyncBit: %u  DelayIn: %u  DelayOut: %u\n",
           i, delay[i], _delay[2*i+1]);
}

void AdcSync::start_training()
{
  _cmd = 1;
  printf("AdcSync set %x\n",_cmd);
}

void AdcSync::stop_training()
{
  _cmd = 0;
  printf("AdcSync set %x\n",_cmd);
}

void AdcSync::dump_status() const
{
  unsigned stats[4];
  for(unsigned j=0; j<4; j++) {
    stats[j] = 0;
    for(unsigned i=0; i<8; i++) {
      usleep(1);
      _select = (j<<16) | i;
      usleep(1);
      stats[j] += _stats;
    }
  }
  printf("---sync stats---\n");
  for(unsigned i=0; i<8; i++) {
    for(unsigned j=0; j<4; j++) {
      usleep(1);
      _select = (j<<16) | i;
      usleep(1);
      unsigned v = _stats;
      if (stats[j])
        printf(" %04x [%02u%%]", v, 100*v/stats[j]);
      else
        printf(" %04x [---]", v);
    }
    printf("\n");
  }
}
