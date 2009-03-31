#include "pds/mon/MonSrc.hh"

using namespace Pds;

MonSrc::MonSrc(unsigned id) : Src(Level::Observer)
{
  _phy=id;
}

MonSrc::~MonSrc() {}

unsigned MonSrc::id() const { return _phy; }
