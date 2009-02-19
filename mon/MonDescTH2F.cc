#include "pds/mon/MonDescTH2F.hh"

using namespace Pds;

MonDescTH2F::MonDescTH2F(const char* name, 
				 const char* xtitle, 
				 const char* ytitle, 
				 unsigned nbinsx, 
				 float xlow, 
				 float xup,
				 unsigned nbinsy, 
				 float ylow, 
				 float yup,
				 bool isnormalized) :
  MonDescEntry(name, xtitle, ytitle, TH2F, sizeof(MonDescTH2F),
		   isnormalized),
  _nbinsx(nbinsx ? nbinsx : 1),
  _nbinsy(nbinsy ? nbinsy : 1),
  _xlow(xlow),
  _xup(xup),
  _ylow(ylow),
  _yup(yup)
{}

void MonDescTH2F::params(unsigned nbinsx, float xlow, float xup,
			     unsigned nbinsy, float ylow, float yup)
{
  _nbinsx = nbinsx ? nbinsx : 1;
  _xlow = xlow;
  _xup = xup;
  _nbinsy = nbinsy ? nbinsy : 1;
  _ylow = ylow;
  _yup = yup;
}
