#include "pds/genericpgp/PeriodMonitor.hh"
#include "pds/service/SysClk.hh"

#include <stdio.h>
#include <string.h>

using namespace Pds::GenericPgp;

PeriodMonitor::PeriodMonitor(unsigned nbins) :
  _size (nbins),
  _histo(new unsigned[nbins]),
  _timeSinceLastException(0),
  _fetchesSinceLastException(0)
{
}

PeriodMonitor::~PeriodMonitor()
{
  delete[] _histo;
}

void PeriodMonitor::clear()
{
  for(unsigned i=0; i<_size; i++)
    _histo[i]=0;
}

void PeriodMonitor::start()
{
  _first=true;
}

void PeriodMonitor::event(unsigned count)
{
  if (_first) {
    _first = false;
    clock_gettime(CLOCK_REALTIME, &_lastTime);
  }
  else {
    clock_gettime(CLOCK_REALTIME, &_thisTime);
    long long unsigned diff = SysClk::diff(_thisTime,_lastTime);
    unsigned peak = 0;
    unsigned max = 0;
    unsigned count = 0;
    diff += 500000;
    diff /= 1000000;
    _timeSinceLastException += (unsigned)(diff & 0xffffffff);
    _fetchesSinceLastException += 1;
    if (diff > _size-1) diff = _size-1;
    _histo[diff] += 1;
    for (unsigned i=0; i<_size; i++) {
      if (_histo[i]) {
	if (_histo[i] > max) {
	  max = _histo[i];
	  peak = i;
	}
	count = 0;
      }
      if (i > count && count > 200) break;
      count += 1;
    }
    if (max > 100) {
      if ( (diff >= ((peak<<1)-(peak>>1))) || (diff <= ((peak>>1))+(peak>>2)) ) {
	printf("Epix10kServer::fetch exceptional period %3llu, not %3u, frame %5u, frames since last %5u, ms since last %5u, ms/f %6.3f\n",
               diff, peak, count, _fetchesSinceLastException, _timeSinceLastException, (1.0*_timeSinceLastException)/_fetchesSinceLastException);
	_timeSinceLastException = 0;
	_fetchesSinceLastException = 0;
      }
    }
    memcpy(&_lastTime, &_thisTime, sizeof(timespec));
  }
}

void PeriodMonitor::print() const
{
  unsigned histoSum = 0;
  printf("Event fetch periods\n");
  for (unsigned i=0; i<_size; i++) {
    if (_histo[i]) {
      printf("\t%3u ms   %8u\n", i, _histo[i]);
      histoSum += _histo[i];
    }
  }
  printf("\tHisto Sum was %u\n", histoSum);
}
