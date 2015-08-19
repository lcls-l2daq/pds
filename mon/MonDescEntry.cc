#include <string.h>

#include "pds/mon/MonDescEntry.hh"

using namespace Pds;

MonDescEntry::MonDescEntry(const char* name,
                           const char* xtitle, 
                           const char* ytitle, 
                           Type type, 
                           unsigned short size,
                           bool isnormalized,
                           bool chartentries) :
  MonDesc(name),
  _group(-1),
  _options((isnormalized ? (1<<Normalized)   : 0) |
           (chartentries ? (1<<ChartEntries) : 0)),
  _type(type),
  _size(size)
{ 
  strncpy(_xtitle, xtitle, TitleSize);
  _xtitle[TitleSize-1] = 0;
  strncpy(_ytitle, ytitle, TitleSize);
  _ytitle[TitleSize-1] = 0;
}

int MonDescEntry::signature() const {
  if (_group >= 0 && id() >= 0) 
    return ((_group << 16) | id());
  else
    return -1;
}

MonDescEntry::Type MonDescEntry::type() const {return Type(_type);}
int short MonDescEntry::group() const  {return _group;}
unsigned short MonDescEntry::size() const {return _size;}
void MonDescEntry::group(int short g) {_group = g;}
const char* MonDescEntry::xtitle() const {return _xtitle;}
const char* MonDescEntry::ytitle() const {return _ytitle;}

bool MonDescEntry::isnormalized() const {return _options&(1<<Normalized);}

void MonDescEntry::xwarnings(float warn, float err) 
{
  _options |= (1<<XWarnings);
  _xwarn = warn;
  _xerr = err;
}

void MonDescEntry::ywarnings(float warn, float err) 
{
  _options |= (1<<YWarnings);
  _ywarn = warn;
  _yerr = err;
}

bool MonDescEntry::xhaswarnings() const {return _options&(1<<XWarnings);}
bool MonDescEntry::yhaswarnings() const {return _options&(1<<YWarnings);}

float MonDescEntry::xwarn() const {return _xwarn;}
float MonDescEntry::xerr() const {return _xerr;}

float MonDescEntry::ywarn() const {return _ywarn;}
float MonDescEntry::yerr() const {return _yerr;}

bool MonDescEntry::chartentries() const {return _options&(1<<ChartEntries);}
