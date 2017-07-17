#include "Module.hh"

#include <unistd.h>
#include <stdio.h>

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


Module::Module()
{
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
void     Module::usLinkUp(unsigned v)  {        setf(_linkUp, v,  6,  0); }

bool     Module::bpLinkUp() const      { return getf(_linkUp,     1, 15); }
void     Module::bpLinkUp(bool v)      {        setf(_linkUp, v,  1, 15); }

unsigned Module::dsLinkUp() const      { return getf(_linkUp,     7, 16); }
bool     Module::dsLinkUp(unsigned v)  {        setf(_linkUp, v,  7, 16); }

void     Module::usLink(unsigned v) const
{
  setf(const_cast<Pds::Cphw::Reg&>(_linkIndices), v, 4,  0);
}
void     Module::dsLink(unsigned v) const
{
  setf(const_cast<Pds::Cphw::Reg&>(_linkIndices), v, 4, 16);
}
void     Module::clearCounters()
{
  setf(_linkIndices, 1, 1, 30);
  usleep(10);
  setf(_linkIndices, 0, 1, 30);
}
void     Module::updateCounters()
{
  setf(_linkIndices, 1, 1, 31);
  usleep(10);
  setf(_linkIndices, 0, 1, 31);
}

unsigned Module::monClkRate() const { return getf(_qpllLock, 29,  0); }
bool     Module::monClkSlow() const { return getf(_qpllLock,  1, 29); }
bool     Module::monClkFast() const { return getf(_qpllLock,  1, 30); }
bool     Module::monClkLock() const { return getf(_qpllLock,  1, 31); }
