#include "pds/mon/MonDescWaveform.hh"

using namespace Pds;

MonDescWaveform::MonDescWaveform(const char* name, 
				 const char* xtitle, 
				 const char* ytitle, 
				 unsigned nbins, 
				 float xlow, 
				 float xup,
				 bool isnormalized) :
  MonDescEntry(name, xtitle, ytitle, Waveform, sizeof(MonDescWaveform),
	       isnormalized),
  _nbins(nbins ? nbins : 1),
  _xlow(xlow),
  _xup(xup)
{}

void MonDescWaveform::params(unsigned nbins, float xlow, float xup)
{
  _nbins = nbins ? nbins : 1;
  _xlow = xlow;
  _xup = xup;
}
