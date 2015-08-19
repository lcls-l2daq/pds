#include "pds/mon/MonDescTH1F.hh"

using namespace Pds;

MonDescTH1F::MonDescTH1F(const char* name, 
                         const char* xtitle, 
                         const char* ytitle, 
                         unsigned nbins, 
                         float xlow, 
                         float xup,
                         bool isnormalized,
                         bool chartentries) :
  MonDescEntry(name, xtitle, ytitle, TH1F, sizeof(MonDescTH1F),
               isnormalized, chartentries),
  _nbins(nbins ? nbins : 1),
  _xlow(xlow),
  _xup(xup)
{}

void MonDescTH1F::params(unsigned nbins, float xlow, float xup)
{
  _nbins = nbins ? nbins : 1;
  _xlow = xlow;
  _xup = xup;
}
