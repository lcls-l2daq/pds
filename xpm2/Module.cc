#include "Module.hh"

#include <unistd.h>
#include <stdio.h>

using namespace Pds::Xpm2;
using Pds::Cphw::Reg;

static inline unsigned getf(const Pds::Cphw::Reg& i, unsigned n, unsigned sh)
{
  unsigned v = i;
  return (v>>sh)&((1<<n)-1);
}

static inline unsigned setf(Pds::Cphw::Reg& o, unsigned v, unsigned n, unsigned sh)
{
  unsigned r = unsigned(o);
  unsigned q = r;
  q &= ~(((1<<n)-1)<<sh);
  q |= (v&((1<<n)-1))<<sh;
  o = q;

  /*
  if (q != unsigned(o)) {
    printf("setf[%p] failed: %08x != %08x\n", &o, unsigned(o), q);
  }
  else if (q != r) {
    printf("setf[%p] passed: %08x [%08x]\n", &o, q, r);
  }
  */
  return q;
}


void L0Stats::dump() const
{
#define PU64(title,stat) printf("%9.9s: %lx\n",#title,stat)
        PU64(Enabled  ,l0Enabled);
        PU64(Inhibited,l0Inhibited);
        PU64(L0,numl0);
        PU64(L0Inh,numl0Inh);
        PU64(L0Acc,numl0Acc);
        for(unsigned i=0; i<32; i+=4)
          printf("Inhibit[%u]: %08x %08x %08x %08x\n", i, 
                 linkInh[i+0],linkInh[i+1],linkInh[i+2],linkInh[i+3]);
#undef PU64
        printf("%9.9s: %x\n","rxErrs",rx0Errs);
}


Module::Module()
{ init(); }

void Module::init()
{
  printf("Module: paddr %x\n", unsigned(_paddr));

  printf("Index:  partition %u  link %u  linkDebug %u  tagStream %u  amc %u  inhibit %u\n",
         getf(_index,4,0),
         getf(_index,6,4),
         getf(_index,6,10),
         getf(_index,1,16),
         getf(_index,1,24),
         getf(_index,2,20));

  unsigned il = getLink();

  printf("%8.8s %8.8s %8.8s %8.8s %8.8s %8.8s %8.8s %8.8s\n",
         "Link", "TxDelay", "Partn", "TrigSrc", "Lb", "txReset", "rxReset", "enable");
  for(unsigned i=0; i<32; i++) {
    setLink(i);
    printf("%8u %8u %8u %8u %8u %8u %8u %8u\n",
           i,
           getf(_dsLinkConfig,20,0),
           getf(_dsLinkConfig,4,20),
           getf(_dsLinkConfig,4,24),
           getf(_dsLinkConfig,1,28),
           getf(_dsLinkConfig,1,29),
           getf(_dsLinkConfig,1,30),
           getf(_dsLinkConfig,1,31));
  }

  printf("%8.8s %8.8s %8.8s %8.8s %8.8s %8.8s %8.8s\n",
         "Link", "RxErr", "TxRstD", "TxRdy", "RxRstD", "RxRdy", "isXpm");
  for(unsigned i=0; i<32; i++) {
    setLink(i);
    printf("%8u %8u %8u %8u %8u %8u %8u\n",
           i,
           getf(_dsLinkStatus,16,0),
           getf(_dsLinkStatus,1,16),
           getf(_dsLinkStatus,1,17),
           getf(_dsLinkStatus,1,18),
           getf(_dsLinkStatus,1,19),
           getf(_dsLinkStatus,1,20));
  }

  setLink(il);

  /*
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
  */
}

void Module::clearLinks()
{
  for(unsigned i=0; i<NDSLinks; i++) {
    setLink(i);
    setf(_dsLinkConfig,0,1,31);
  }
}

void Module::linkEnable(unsigned link, bool v)
{
  setLink(link);
  usleep(10);
  unsigned q = _dsLinkConfig;
  setf(_dsLinkConfig,v?1:0,1,31);
  unsigned r = _dsLinkConfig;
  printf("linkEnable[%u,%c] %x -> %x\n", link,v?'T':'F',q,r);
}

void Module::linkLoopback(unsigned link, bool v)
{
  setLink(link);
  setf(_dsLinkConfig,v?1:0,1,28);
}

void Module::txLinkReset(unsigned link)
{
  setLink(link);
  setf(_dsLinkConfig,1,1,29);
  usleep(10);
  setf(_dsLinkConfig,0,1,29);
}

void Module::rxLinkReset(unsigned link)
{
  setLink(link);
  setf(_dsLinkConfig,1,1,30);
  usleep(10);
  setf(_dsLinkConfig,0,1,30);
}

bool Module::l0Enabled() const { return getf(_l0Control,1,16); }

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
  //  for(unsigned i=0; i<NDSLinks; i++) {
  for(unsigned i=0; i<32; i++) {
    setLink(i);
    // if (getf(_dsLinkConfig,1,31))
       s.linkInh[i] = _inhibitCounts[i];
  }
  //  Release the counters
  const_cast<Module&>(*this).lockL0Stats(false);

  s.rx0Errs = rxLinkErrs(0);

  return s;
}

unsigned Module::txLinkStat() const
{ return 0; }

unsigned Module::rxLinkStat() const
{ return 0; }

unsigned Module::rxLinkErrs(unsigned link) const
{
  setLink(link);
  return getf(_dsLinkStatus,16,0);
}

void Module::resetL0(bool v)
{
  setf(_l0Control,v?1:0,1,0);
}

void Module::setL0Enabled(bool v)
{
  setf(_l0Control,v?1:0,1,16);
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
  setf(_l0Control,v?0:1,1,31);
}

void Module::setRingBChan(unsigned chan)
{
  setf(_index,chan,4,10);
}

void Module::pllBwSel    (unsigned sel)
{
  setf(_pllConfig,sel,4,0);
}

void Module::pllFrqSel    (unsigned sel)
{
  setf(_pllConfig,sel,12,4);
}

void Module::pllRateSel    (unsigned sel)
{
  setf(_pllConfig,sel,4,16);
}

void Module::pllBypass   (bool v)
{
  setf(_pllConfig,v?1:0,1,22);
}
  
void Module::pllReset    ()
{
  setf(_pllConfig,0,1,23);
  usleep(10);
  setf(_pllConfig,1,1,23);
}

void Module::pllSkew(int skewSteps)
{
  unsigned v = _pllConfig;
  while(skewSteps > 0) {
    _pllConfig = v|(1<<20);
    usleep(10);
    _pllConfig = v;
    usleep(10);
    skewSteps--;
  }
  while(skewSteps < 0) {
    _pllConfig = v|(1<<21);
    usleep(10);
    _pllConfig = v;
    usleep(10);
    skewSteps++;
  }
}

void Module::dumpPll() const
{
  printf("pllConfig 0x%08x\n",
         unsigned(_pllConfig));
}

void Module::setPartition(unsigned v) { setf(_index,v,4,0); }

void Module::setAmc(unsigned v) { setf(_index,v,1,16); }

void Module::setLink(unsigned v) const 
{
  setf(const_cast<Pds::Cphw::Reg&>(_index),v,6,4); 
}

unsigned Module::getPartition() const { return getf(_index,4,0); }

unsigned Module::getAmc() const { return getf(_index,1,16); }

unsigned Module::getLink() const { return getf(_index,6,4); }
