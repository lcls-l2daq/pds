#include "Module.hh"

#include "pds/cphw/HsRepeater.hh"

#include <unistd.h>
#include <stdio.h>
#include <new>

using namespace Pds::Dti;
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
  return q;
}


void Stats::dump() const
{
#define PU32(title,stat) printf("%9.9s: %x\n",#title,stat)

  printf("Link Up:  US: %x  BP: %x  DS: %x\n", usLinkUp, bpLinkUp, dsLinkUp);

  printf("UsLink  RxErrs    RxFull    IbRecv    IbEvt     ObRecv    ObSent\n");
  //      0      00000000  00000000  00000000  00000000  00000000  00000000
  for (unsigned i = 0; i < Module::NUsLinks; ++i)
  {
    printf("%d      %08x  %08x  %08x  %08x  %08x  %08x\n", i,
           us[i].rxErrs,
           us[i].rxFull,
           us[i].ibRecv,
           us[i].ibEvt,
           us[i].obRecv,
           us[i].obSent);
  }

  printf("DsLink  RxErrs    RxFull      ObSent\n");
  //      0      00000000  00000000  000000000000
  for (unsigned i = 0; i < Module::NDsLinks; ++i)
  {
    printf("%d      %08x  %08x  %012lx\n", i,
           ds[i].rxErrs,
           ds[i].rxFull,
           ds[i].obSent);
  }

  PU32(qpllLock, qpllLock);

  printf("MonClk   Rate   Slow  Fast  Lock\n");
  //      0      00000000  0     0     0
  for (unsigned i = 0; i < 4; ++i)
  {
    printf("%d  %08x  %d     %d     %d\n", i,
           monClk[i].rate,
           monClk[i].slow,
           monClk[i].fast,
           monClk[i].lock);
  }

  printf("usLinkOb  L0 %08x  L1A %08x  L1R %08x\n", usLinkObL0, usLinkObL1A, usLinkObL1R);
}


Module* Module::module()
{
  return new((void*)0x80000000) Module;
}

Pds::Cphw::HsRepeater* Module::hsRepeater()
{
  return new((void*)0x09000000) Pds::Cphw::HsRepeater;
}

Module::Module()
{
  clearCounters();
  updateCounters();                     // Revisit: Necessary?
}


bool     UsLink::enabled() const       { return getf(_config,     1,  0); }
void     UsLink::enable(bool v)        {        setf(_config, v,  1,  0); }

bool     UsLink::tagEnabled() const    { return getf(_config,     1,  1); }
void     UsLink::tagEnable(bool v)     {        setf(_config, v,  1,  1); }

bool     UsLink::l1Enabled() const     { return getf(_config,     1,  2); }
void     UsLink::l1Enable(bool v)      {        setf(_config, v,  1,  2); }

unsigned UsLink::partition() const     { return getf(_config,     4,  4); }
void     UsLink::partition(unsigned v) {        setf(_config, v,  4,  4); }

unsigned UsLink::trigDelay() const     { return getf(_config,     8,  8); }
void     UsLink::trigDelay(unsigned v) {        setf(_config, v,  8,  8); }

unsigned UsLink::fwdMask() const       { return getf(_config,    13, 16); }
void     UsLink::fwdMask(unsigned v)   {        setf(_config, v, 13, 16); }

bool     UsLink::fwdMode() const       { return getf(_config,     1, 31); }
void     UsLink::fwdMode(bool v)       {        setf(_config, v,  1, 31); }


unsigned Module::usLinkUp() const      { return getf(_linkUp,     6,  0); }
bool     Module::bpLinkUp() const      { return getf(_linkUp,     1, 15); }
unsigned Module::dsLinkUp() const      { return getf(_linkUp,     7, 16); }

void     Module::usLink(unsigned v) const
{
  setf(const_cast<Pds::Cphw::Reg&>(_linkIndices), v, 4,  0);
}
void     Module::dsLink(unsigned v) const
{
  setf(const_cast<Pds::Cphw::Reg&>(_linkIndices), v, 4, 16);
}
void     Module::clearCounters() const
{
  setf(const_cast<Pds::Cphw::Reg&>(_linkIndices), 1, 1, 30);
  usleep(10);
  setf(const_cast<Pds::Cphw::Reg&>(_linkIndices), 0, 1, 30);
}
void     Module::updateCounters() const
{
  setf(const_cast<Pds::Cphw::Reg&>(_linkIndices), 1, 1, 31);
  usleep(10);
  setf(const_cast<Pds::Cphw::Reg&>(_linkIndices), 0, 1, 31);
}

unsigned Module::monClkRate(unsigned i) const { return getf(_monClk[i], 29,  0); }
bool     Module::monClkSlow(unsigned i) const { return getf(_monClk[i],  1, 29); }
bool     Module::monClkFast(unsigned i) const { return getf(_monClk[i],  1, 30); }
bool     Module::monClkLock(unsigned i) const { return getf(_monClk[i],  1, 31); }

Stats Module::stats() const
{
  Stats s;

  // Revisit: Useful?  s.enabled    = enabled();
  // Revisit: Useful?  s.tagEnabled = tagEnabled();
  // Revisit: Useful?  s.l1Enabled  = l1Enabled();

  s.usLinkUp = usLinkUp();
  s.bpLinkUp = bpLinkUp();
  s.dsLinkUp = dsLinkUp();

  for (unsigned i = 0; i < NUsLinks; ++i)
  {
    usLink(i);

    s.us[i].rxErrs = _usRxErrs;
    s.us[i].rxFull = _usRxFull;
    s.us[i].ibRecv = _usIbRecv;
    s.us[i].ibEvt  = _usIbEvt;
    s.us[i].obRecv = _usObRecv;
    s.us[i].obSent = _usObSent;
  }

  for (unsigned i = 0; i < NDsLinks; ++i)
  {
    dsLink(i);

    s.ds[i].rxErrs = _dsRxErrs;
    s.ds[i].rxFull = _dsRxFull;
    s.ds[i].obSent = _dsObSent;
  }

  s.qpllLock = _qpllLock;

  updateCounters();                     // Revisit: Is this needed?

  for (unsigned i = 0; i < 4; ++i)
  {
    s.monClk[i].rate = monClkRate(i);
    s.monClk[i].slow = monClkSlow(i);
    s.monClk[i].fast = monClkFast(i);
    s.monClk[i].lock = monClkLock(i);
  }

  s.usLinkObL0  = _usLinkObL0;
  s.usLinkObL1A = _usLinkObL1A;
  s.usLinkObL1R = _usLinkObL1R;

  return s;
}
