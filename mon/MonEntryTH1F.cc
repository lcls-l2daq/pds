#include "pds/mon/MonEntryTH1F.hh"

static const unsigned DefaultNbins = 1;
static const float DefaultLo = 0;
static const float DefaultUp = 1;

using namespace Pds;

#define SIZE(nb) (nb+InfoSize)

MonEntryTH1F::~MonEntryTH1F() {}

MonEntryTH1F::MonEntryTH1F(const char* name, 
			   const char* xtitle, 
			   const char* ytitle,
			   bool isnormalized) :
  _desc(name, xtitle, ytitle, DefaultNbins, DefaultLo, DefaultUp, isnormalized)
{
  build(DefaultNbins);
}

MonEntryTH1F::MonEntryTH1F(const MonDescTH1F& desc) :
  _desc(desc)
{
  build(desc.nbins());
}

void MonEntryTH1F::params(unsigned nbins, float xlow, float xup)
{
  _desc.params(nbins, xlow, xup);
  build(nbins);
}

void MonEntryTH1F::params(const MonDescTH1F& desc)
{
  _desc = desc;
  build(_desc.nbins());
}

void MonEntryTH1F::build(unsigned nbins)
{
  _y = static_cast<double*>(allocate(sizeof(double)*SIZE(nbins)));
}

const MonDescTH1F& MonEntryTH1F::desc() const {return _desc;}
MonDescTH1F& MonEntryTH1F::desc() {return _desc;}

void MonEntryTH1F::setto(const MonEntryTH1F& entry) 
{
  double* dst = _y;
  const double* end = dst+SIZE(_desc.nbins());
  const double* src = entry._y;
  do {
    *dst++ = *src++;
  } while (dst < end);
  time(entry.time());
}

void MonEntryTH1F::setto(const MonEntryTH1F& curr, 
			 const MonEntryTH1F& prev) 
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

void MonEntryTH1F::stats() 
{
  MonStats1D::stats(_desc.nbins(), _desc.xlow(), _desc.xup(), _y);
}

void MonEntryTH1F::addcontent(double y, double x)
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

