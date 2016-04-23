#include "Module.hh"

#include <unistd.h>
#include <stdio.h>

using namespace Xpm;

Module::Module()
{ init(); }

void Module::init()
{
  printf("Module\n");
  printf("enabled  %x\n", unsigned(_enabled));
  printf("l0Select %x\n", unsigned(_l0Select));
  printf("txLinkSt %x\n", unsigned(_dsTxLinkStatus));
  printf("rxLinkSt %x\n", unsigned(_dsRxLinkStatus));
  printf("l0Enabld %x\n", unsigned(_l0Enabled));
  printf("l0Inh    %x\n", unsigned(_l0Inhibited));
  printf("numl0    %x\n", unsigned(_numl0));
  printf("numl0Inh %x\n", unsigned(_numl0Inh));
  printf("numl0Acc %x\n", unsigned(_numl0Acc));
}

void Module::linkEnable(unsigned link, bool v)
{
  if (v)
    _dsLinkEnable.setBit(link);
  else
    _dsLinkEnable.clearBit(link);
}

void Module::txLinkReset(unsigned link)
{
  unsigned b = link&0xf;
  _dsLinkReset.setBit(b);
  usleep(10);
  _dsLinkReset.clearBit(b);
}

void Module::rxLinkReset(unsigned link)
{
  unsigned b=(link&0xf)+16;
  _dsLinkReset.setBit(b);
  usleep(10);
  _dsLinkReset.clearBit(b);
}

bool Module::l0Enabled() const { return unsigned(_enabled)!=0; }

L0Stats Module::l0Stats() const
{
  L0Stats s;
  s.l0Enabled   = _l0Enabled;
  s.l0Inhibited = _l0Inhibited;
  s.numl0       = _numl0;
  s.numl0Inh    = _numl0Inh;
  s.numl0Acc    = _numl0Acc;
  s.rx0Errs     = (_dsRxLinkStatus>>16)&0x3fff;
  return s;
}

unsigned Module::txLinkStat() const
{
  return unsigned(_dsTxLinkStatus);
}

unsigned Module::rxLinkStat() const
{
  return unsigned(_dsRxLinkStatus);
}

void Module::setL0Enabled(bool v)
{
  _enabled = v ? 1:0;
}

void Module::setL0Select_FixedRate(unsigned rate)
{
  unsigned rateSel = (0<<14) | (rate&0xf);
  unsigned destSel = _l0Select >> 16;
  _l0Select = (destSel<<16) | rateSel;
}

void Module::setL0Select_ACRate   (unsigned rate, unsigned tsmask)
{
  unsigned rateSel = (1<<14) | ((tsmask&0x3f)<<3) | (rate&0x7);
  unsigned destSel = _l0Select >> 16;
  _l0Select = (destSel<<16) | rateSel;
}

void Module::setL0Select_Sequence (unsigned seq , unsigned bit)
{
  unsigned rateSel = (2<<14) | ((seq&0x3f)<<8) | (bit&0xf);
  unsigned destSel = _l0Select >> 16;
  _l0Select = (destSel<<16) | rateSel;
}
