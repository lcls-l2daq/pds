#include "pds/mon/MonEntryWaveform.hh"

static const unsigned DefaultNbins = 1;
static const float DefaultLo = 0;
static const float DefaultUp = 1;

using namespace Pds;

#define SIZE(nb) (nb+InfoSize)

MonEntryWaveform::~MonEntryWaveform() {}

MonEntryWaveform::MonEntryWaveform(const char* name, 
				   const char* xtitle, 
				   const char* ytitle,
				   bool isnormalized) :
  _desc(name, xtitle, ytitle, DefaultNbins, DefaultLo, DefaultUp, isnormalized)
{
  build(DefaultNbins);
}

MonEntryWaveform::MonEntryWaveform(const MonDescWaveform& desc) :
  _desc(desc)
{
  build(desc.nbins());
}

void MonEntryWaveform::params(unsigned nbins, float xlow, float xup)
{
  delete[] _y;
  _desc.params(nbins, xlow, xup);
  build(nbins);
}

void MonEntryWaveform::params(const MonDescWaveform& desc)
{
  delete[] _y;
  _desc = desc;
  build(_desc.nbins());
}

void MonEntryWaveform::build(unsigned nbins)
{
  _y = static_cast<double*>(allocate(sizeof(double)*SIZE(nbins)));
  stats();
}

const MonDescWaveform& MonEntryWaveform::desc() const {return _desc;}
MonDescWaveform& MonEntryWaveform::desc() {return _desc;}

void MonEntryWaveform::setto(const MonEntryWaveform& entry) 
{
  double* dst = _y;
  const double* end = dst+SIZE(_desc.nbins());
  const double* src = entry._y;
  do {
    *dst++ = *src++;
  } while (dst < end);
  time(entry.time());
}

void MonEntryWaveform::setto(const MonEntryWaveform& curr, 
			     const MonEntryWaveform& prev) 
{
  double* dst = _y;
  const double* end = dst+SIZE(_desc.nbins());
  const double* srccurr = curr._y;
  const double* srcprev = prev._y;
  do {
    *dst++ = *srccurr++ - *srcprev++;
  } while (dst < end);
  time(curr.time());
}

void MonEntryWaveform::stats() 
{
  MonStats1D::stats(_desc.nbins(), _desc.xlow(), _desc.xup(), _y);
}

void MonEntryWaveform::addcontent(double y, double x)
{
  if (x < _desc.xlow()) 
    addinfo(y, Underflow);
  else if (x >= _desc.xup()) 
    addinfo(y, Overflow);
  else {
    unsigned bin = unsigned(((x-_desc.xlow())*double(_desc.nbins()) / 
			     (_desc.xup()-_desc.xlow())));
    addcontent(y,bin);
  }
}

