#include "pds/mon/MonEntryTH2F.hh"

static const unsigned DefaultNbins = 1;
static const float DefaultLo = 0;
static const float DefaultUp = 1;

using namespace Pds;

#define SIZE(nx,ny) (nx*ny+InfoSize)

MonEntryTH2F::~MonEntryTH2F() {}

MonEntryTH2F::MonEntryTH2F(const char* name, 
			   const char* xtitle, 
			   const char* ytitle,
			   bool isnormalized) :
  _desc(name, xtitle, ytitle, 
	DefaultNbins, DefaultLo, DefaultUp,
	DefaultNbins, DefaultLo, DefaultUp,
	isnormalized)
{
  build(DefaultNbins, DefaultNbins);
}

MonEntryTH2F::MonEntryTH2F(const MonDescTH2F& desc) :
  _desc(desc)
{
  build(_desc.nbinsx(), _desc.nbinsy());
}

void MonEntryTH2F::params(unsigned nbinsx, float xlow, float xup,
			  unsigned nbinsy, float ylow, float yup)
{
  _desc.params(nbinsx, xlow, xup, nbinsy, ylow, yup);
  build(nbinsx, nbinsy);
}

void MonEntryTH2F::params(const MonDescTH2F& desc)
{
  _desc = desc;
  build(_desc.nbinsx(), _desc.nbinsy());
}

void MonEntryTH2F::build(unsigned nbinsx, unsigned nbinsy)
{
  _y = static_cast<float*>(allocate(sizeof(float)*SIZE(nbinsx,nbinsy)));
}

const MonDescTH2F& MonEntryTH2F::desc() const {return _desc;}
MonDescTH2F& MonEntryTH2F::desc() {return _desc;}

void MonEntryTH2F::setto(const MonEntryTH2F& entry) 
{
  float* dst = _y;
  const float* end = dst+SIZE(_desc.nbinsx(),_desc.nbinsy());
  const float* src = entry._y;
  do {
    *dst++ = *src++;
  } while (dst < end);
  time(entry.time());
}

void MonEntryTH2F::setto(const MonEntryTH2F& curr, 
			 const MonEntryTH2F& prev)
{
  float* dst = _y;
  const float* end = dst+SIZE(_desc.nbinsx(),_desc.nbinsy());
  const float* srccurr = curr._y;
  const float* srcprev = prev._y;
  do {
    *dst++ = *srccurr++ - *srcprev++;
  } while (dst < end);
  time(curr.time());
}

void MonEntryTH2F::stats() 
{
  MonStats2D::stats(_desc.nbinsx(), _desc.nbinsy(), 
		    _desc.xlow(), _desc.xup(),
		    _desc.ylow(), _desc.yup(), _y);
}
