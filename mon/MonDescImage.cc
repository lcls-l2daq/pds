#include "pds/mon/MonDescImage.hh"

using namespace Pds;

MonDescImage::MonDescImage(const char* name, 
			   unsigned nbinsx, 
			   unsigned nbinsy, 
			   int ppbx,
			   int ppby) :
  MonDescEntry(name, "x", "y", Image, sizeof(MonDescImage)),
  _nbinsx(nbinsx ? nbinsx : 1),
  _nbinsy(nbinsy ? nbinsy : 1),
  _ppbx  (ppbx),
  _ppby  (ppby)
{}

void MonDescImage::params(unsigned nbinsx,
			  unsigned nbinsy,
			  int ppxbin,
			  int ppybin)
{
  _nbinsx = nbinsx ? nbinsx : 1;
  _nbinsy = nbinsy ? nbinsy : 1;
  _ppbx = ppxbin;
  _ppby = ppybin;
}
