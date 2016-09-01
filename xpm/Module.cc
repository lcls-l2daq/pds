#include "Module.hh"

#include <unistd.h>
#include <stdio.h>

using namespace Pds::Xpm;
using Pds::Cphw::Reg;

void L0Stats::dump() const
{
#define PU64(title,stat) printf("%9.9s: %lx\n",#title,stat)
        PU64(Enabled  ,l0Enabled);
        PU64(Inhibited,l0Inhibited);
        PU64(L0,numl0);
        PU64(L0Inh,numl0Inh);
        PU64(L0Acc,numl0Acc);
        for(unsigned i=0; i<8; i++)
          printf("Inhibit[%u]: %x\n", i, linkInh[i]);
#undef PU64
        printf("%9.9s: %x\n","rxErrs",rx0Errs);
}


Module::Module()
{ init(); }

void Module::init()
{
  printf("Module\n");
  printf("l0 enabled [%x]  reset [%x]\n", 
         unsigned(_enabled)>>16, unsigned(_enabled)&0xffff);
  printf("l0 rate [%x]  dest [%x]\n", 
         unsigned(_l0Select)&0xffff, unsigned(_l0Select)>>16);
  printf("link  fullEn [%x]  loopEn [%x]\n", 
         unsigned(_dsLinkEnable)&0xffff,
         unsigned(_dsLinkEnable)>>16);
  printf("txReset PMA [%x]  All [%x]\n", 
         unsigned(_dsTxLinkStatus)&0xffff,
         unsigned(_dsTxLinkStatus)>>16);
  printf("rxReset PMA [%x]  All [%x]\n", 
         unsigned(_dsRxLinkStatus)&0xffff,
         unsigned(_dsRxLinkStatus)>>16);
  printf("l0Enabld %x\n", unsigned(_l0Enabled));
  printf("l0Inh    %x\n", unsigned(_l0Inhibited));
  printf("numl0    %x\n", unsigned(_numl0));
  printf("numl0Inh %x\n", unsigned(_numl0Inh));
  printf("numl0Acc %x\n", unsigned(_numl0Acc));
}

void Module::clearLinks()
{
  unsigned b = _dsLinkEnable;
  _dsLinkEnable = b&0xffff0000;
}

void Module::linkEnable(unsigned link, bool v)
{
  if (v)
    _dsLinkEnable.setBit(link);
  else
    _dsLinkEnable.clearBit(link);
}

void Module::linkLoopback(unsigned link, bool v)
{
  if (v)
    _dsLinkEnable.setBit(link+16);
  else
    _dsLinkEnable.clearBit(link+16);
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

bool Module::l0Enabled() const { return unsigned(_enabled&0x10001)==0x10000; }

L0Stats Module::l0Stats() const
{
  //  Lock the counters
  const_cast<Module&>(*this).lockL0Stats(true);
  L0Stats s;
  s.l0Enabled   = _l0Enabled;
  s.l0Inhibited = _l0Inhibited;
  s.numl0       = _numl0;
  s.numl0Inh    = _numl0Inh;
  s.numl0Acc    = _numl0Acc;
  unsigned mask = _dsLinkEnable&0x00ff;
  for(unsigned i=0; mask; i++)
    if (mask&(1<<i)) {
      mask &= ~(1<<i);
      s.linkInh[i] = _dsLinkInh[i];
    }
  //  Release the counters
  const_cast<Module&>(*this).lockL0Stats(false);

  /*
  unsigned e=_dsLinkEnable&0xffff;
  unsigned v=0;
  { for(unsigned i=0; e!=0; i++)
      if (e & (1<<i)) {
        e ^= (1<<i);
        v += rxLinkErrs(i);
      } }
  s.rx0Errs     = v;
  */
  s.rx0Errs = rxLinkErrs(0);

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

unsigned Module::rxLinkErrs(unsigned link) const
{
  unsigned v=_rxLinkErrs[link>>1];
  return (v >> (16*(link&1))) & 0xffff;
}

void Module::resetL0(bool v)
{
  unsigned r = _enabled;
  if (v)
    _enabled = r|1;
  else
    _enabled = r&~1;
}

void Module::setL0Enabled(bool v)
{
  if (v)
    _enabled = 0x10000;
  else
    _enabled = 0;
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

void Module::lockL0Stats(bool v)
{
  unsigned r = _enabled;
  if (v)
    _enabled = r&~(1<<31);
  else
    _enabled = r|(1<<31);
}

void Module::setRingBChan(unsigned chan)
{
  unsigned r = _pllConfig0;
  r &= ~0x0f000000;
  r |= ((chan&0x7)<<24);
  _pllConfig0 = r;
}

void Module::pllBwSel    (unsigned sel)
{
  unsigned r = _pllConfig0;
  r &= ~0xf;
  r |= (sel&0xf);
  _pllConfig0 = r;
}

void Module::pllFrqSel    (unsigned sel)
{
  unsigned r = _pllConfig0;
  r &= ~0xfff0;
  r |= (sel&0xfff)<<4;
  _pllConfig0 = r;
}

void Module::pllRateSel    (unsigned sel)
{
  unsigned r = _pllConfig0;
  r &= ~0xf0000;
  r |= (sel&0xf)<<16;
  _pllConfig0 = r;
}

void Module::pllBypass   (bool v)
{
  unsigned r = _pllConfig0;
  if (v)
    r |= (1<<22);
  else
    r &= ~(1<<22);
  _pllConfig0 = r;
}
  
void Module::pllReset    ()
{
  unsigned r = _pllConfig0;
  _pllConfig0 = r & ~(1<<23);
  usleep(10);
  _pllConfig0 = r | (1<<23);
}

void Module::pllSkew(int skewSteps)
{
  unsigned v = _pllConfig0;
  while(skewSteps > 0) {
    _pllConfig0 = v|(1<<20);
    usleep(10);
    _pllConfig0 = v;
    usleep(10);
    skewSteps--;
  }
  while(skewSteps < 0) {
    _pllConfig0 = v|(1<<21);
    usleep(10);
    _pllConfig0 = v;
    usleep(10);
    skewSteps++;
  }
}
