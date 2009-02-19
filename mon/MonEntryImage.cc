#include "pds/mon/MonEntryImage.hh"

static const unsigned DefaultNbins = 1;
static const float DefaultLo = 0;
static const float DefaultUp = 1;

using namespace Pds;

#define SIZE(nx,ny) (nx*ny+InfoSize)

MonEntryImage::~MonEntryImage() {}

MonEntryImage::MonEntryImage(const char* name) :
  _desc(name, DefaultNbins, DefaultNbins)
{
  build(DefaultNbins, DefaultNbins);
}

MonEntryImage::MonEntryImage(const MonDescImage& desc) :
  _desc(desc)
{
  build(_desc.nbinsx(), _desc.nbinsy());
}

void MonEntryImage::params(unsigned nbinsx,
			   unsigned nbinsy,
			   int ppxbin,
			   int ppybin)
{
  _desc.params(nbinsx, nbinsy, ppxbin, ppybin);
  build(nbinsx, nbinsy);
}

void MonEntryImage::params(const MonDescImage& desc)
{
  _desc = desc;
  build(_desc.nbinsx(), _desc.nbinsy());
}

void MonEntryImage::build(unsigned nbinsx, unsigned nbinsy)
{
  _y = static_cast<unsigned*>(allocate(sizeof(unsigned)*SIZE(nbinsx,nbinsy)));
}

const MonDescImage& MonEntryImage::desc() const {return _desc;}
MonDescImage& MonEntryImage::desc() {return _desc;}

void MonEntryImage::setto(const MonEntryImage& entry) 
{
  unsigned* dst = _y;
  const unsigned* end = dst+SIZE(_desc.nbinsx(),_desc.nbinsy());
  const unsigned* src = entry._y;
  do {
    *dst++ = *src++;
  } while (dst < end);
  time(entry.time());
}

void MonEntryImage::setto(const MonEntryImage& curr, 
			  const MonEntryImage& prev)
{
  unsigned* dst = _y;
  const unsigned* end = dst+SIZE(_desc.nbinsx(),_desc.nbinsy());
  const unsigned* srccurr = curr._y;
  const unsigned* srcprev = prev._y;
  do {
    *dst++ = *srccurr++ - *srcprev++;
  } while (dst < end);
  time(curr.time());
}

