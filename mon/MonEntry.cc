#include <string.h>
#include <sys/uio.h>

#include "pds/mon/MonEntry.hh"
#include "pdsdata/xtc/ClockTime.hh"

using namespace Pds;

MonEntry::MonEntry() : 
  _payloadsize(0),
  _payload(0)
{}

MonEntry::~MonEntry()
{
  delete [] _payload;  
}

void MonEntry::payload(iovec& iov)
{
  iov.iov_base = _payload;
  iov.iov_len = _payloadsize;
}

void MonEntry::payload(iovec& iov) const
{
  iov.iov_base = static_cast<void*>(_payload);
  iov.iov_len = _payloadsize;
}

void MonEntry::reset()
{
  memset(_payload, 0, _payloadsize);
}

void* MonEntry::allocate(unsigned size)
{
  if (_payload)
    delete [] _payload;  

  _payloadsize = sizeof(unsigned long long)+size;
  _payload = new unsigned long long[(_payloadsize>>3)+1];

  reset();

  return (_payload+1);
}

double MonEntry::last() const 
{
  const ClockTime& t = time();
  return t.seconds() + 1.e-9 * t.nanoseconds();
}

const ClockTime& MonEntry::time() const 
{
  return *reinterpret_cast<const ClockTime*>(_payload);
}

void MonEntry::time(const ClockTime& t) 
{
  *_payload = *(reinterpret_cast<const unsigned long long*>(&t));
}

