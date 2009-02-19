#include <string.h>
#include <sys/uio.h>

#include "pds/mon/MonUsage.hh"

static const unsigned Step = 32;

using namespace Pds;

MonUsage::MonUsage() :
  _signatures(new int[Step]),
  _usage(new unsigned short[Step]),
  _used(0),
  _maxused(Step),
  _ismodified(false)
{}

MonUsage::~MonUsage() 
{
  delete [] _signatures;
  delete [] _usage;
}

int MonUsage::use(int signature)
{
  for (unsigned short u=0; u<_used; u++) {
    if (_signatures[u] == signature) {
      return ++_usage[u];
    }
  }
  if (_used == _maxused) adjust();
  _signatures[_used] = signature;
  _usage[_used] = 1;
  _used++;
  _ismodified = true;
  return 1;
}

int MonUsage::dontuse(int signature)
{
  for (unsigned short u=0; u<_used; u++) { 
    if (_signatures[u] == signature) {
      if (--_usage[u] == 0) {
	unsigned short p = u+1;
	int d = _used - p;
	if (d) {
	  memmove(_signatures+u, _signatures+p, d*sizeof(_signatures[0]));
	  memmove(_usage+u, _usage+p, d*sizeof(_usage[0]));
	}
	_used--;
	_ismodified = true;
	return 0;
      }
      return _usage[u];
    }
  }
  return -1;
}

void MonUsage::reset()
{
  _used = 0;
  _ismodified = true;
}

void MonUsage::adjust()
{
  unsigned short maxused = _maxused+Step;
  int* signatures = new int[maxused];
  unsigned short* usage = new unsigned short[maxused];
  memcpy(signatures, _signatures, _used*sizeof(int));
  memcpy(usage, _usage, _used*sizeof(int));
  delete [] _signatures;
  delete [] _usage;
  _signatures = signatures;
  _usage = usage;
  _maxused = maxused;
}

void MonUsage::request(iovec& iov)
{
  iov.iov_base = _signatures;
  iov.iov_len = _used<<2;
  _ismodified = false;
}

unsigned short MonUsage::used() const {return _used;}
int MonUsage::signature(unsigned short u) const {return _signatures[u];}
bool MonUsage::ismodified() const {return _ismodified;}
