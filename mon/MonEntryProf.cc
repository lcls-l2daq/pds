#include "pds/mon/MonEntryProf.hh"

static const unsigned DefaultNbins = 1;
static const float DefaultLo = 0;
static const float DefaultUp = 1;
static const char* DefaultNames = 0;

using namespace Pds;

#define SIZE(nb) (3*nb+InfoSize)

MonEntryProf::~MonEntryProf() {}

MonEntryProf::MonEntryProf(const char* name, 
			   const char* xtitle, 
			   const char* ytitle) :
  _desc(name, xtitle, ytitle, DefaultNbins, DefaultLo, DefaultUp, DefaultNames)
{
  build(DefaultNbins);
}

MonEntryProf::MonEntryProf(const MonDescProf& desc) :
  _desc(desc)
{
  build(desc.nbins());
}

void MonEntryProf::params(unsigned nbins, 
			  float xlow, 
			  float xup, 
			  const char* names)
{
  _desc.params(nbins, xlow, xup, names);
  build(nbins);
}

void MonEntryProf::params(const MonDescProf& desc)
{
  _desc = desc;
  build(_desc.nbins());
}

void MonEntryProf::build(unsigned nbins)
{
  _ysum = static_cast<double*>(allocate(SIZE(nbins)*sizeof(double)));
  _y2sum = _ysum+nbins;
  _nentries = _y2sum+nbins;
}

const MonDescProf& MonEntryProf::desc() const {return _desc;}
MonDescProf& MonEntryProf::desc() {return _desc;}

void MonEntryProf::setto(const MonEntryProf& entry) 
{
  double* dst = _ysum;
  const double* end = dst + SIZE(_desc.nbins());
  const double* src = entry._ysum;
  do {
    *dst++ = *src++;
  } while (dst < end);
  time(entry.time());
}

void MonEntryProf::setto(const MonEntryProf& curr, 
			 const MonEntryProf& prev) 
{
  double* dst = _ysum;
  const double* end = dst + SIZE(_desc.nbins());
  const double* srccurr = curr._ysum;
  const double* srcprev = prev._ysum;
  do {
    *dst++ = *srccurr++ - *srcprev++;
  } while (dst < end);
  time(curr.time());
}
