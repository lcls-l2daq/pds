#include "pds/mon/MonDescProf.hh"

using namespace Pds;

MonDescProf::MonDescProf(const char* name, 
				 const char* xtitle, 
				 const char* ytitle, 
				 unsigned nbins, 
				 float xlow, 
				 float xup, 
				 const char* names) :
  MonDescEntry(name, xtitle, ytitle, Prof, sizeof(MonDescProf)),
  _nbins(nbins ? nbins : 1),
  _xlow(xlow),
  _xup(xup)
{
  if (names) {
    char* dst = _names;
    const char* last = dst+NamesSize-1;
    while (*names && dst < last) {
      *dst = *names != ':' ? *names : 0;
      dst++;
      names++;
    }
    *dst++ = 0;
    *dst++ = 0;
  } else {
    _names[0] = 0;
  }
}

void MonDescProf::params(unsigned nbins, 
			     float xlow, 
			     float xup, 
			     const char* names)
{
  _nbins = nbins ? nbins : 1;
  _xlow = xlow;
  _xup = xup;
  if (names) {
    char* dst = _names;
    const char* last = dst+NamesSize-1;
    while (*names && dst < last) {
      *dst = *names != ':' ? *names : 0;
      dst++;
      names++;
    }
    *dst++ = 0;
    *dst++ = 0;
  } else {
    _names[0] = 0;
  }
}
