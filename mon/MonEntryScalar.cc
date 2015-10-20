#include "pds/mon/MonEntryScalar.hh"

#include <stdio.h>

using namespace Pds;

MonEntryScalar::~MonEntryScalar() {}

MonEntryScalar::MonEntryScalar(const MonDescScalar& desc) :
  _desc(desc)
{
  _y = static_cast<double*>(allocate(sizeof(double)*_desc.elements()));
}

const MonDescScalar& MonEntryScalar::desc() const {return _desc;}
MonDescScalar& MonEntryScalar::desc() {return _desc;}

void MonEntryScalar::params(const MonDescScalar& desc) {
  _desc=desc;
  _y = static_cast<double*>(allocate(sizeof(double)*_desc.elements()));
}

std::vector<double> MonEntryScalar::values() const
{
  std::vector<double> v(_desc.elements());
  for(unsigned i=0; i<_desc.elements(); i++)
    v[i] = _y[i];

  return v;
}

std::vector<double> MonEntryScalar::since(const MonEntryScalar& o) const
{
  std::vector<double> v(_desc.elements());
  for(unsigned i=0; i<_desc.elements(); i++)
    v[i] = _y[i]-o._y[i];

  return v;
}

void MonEntryScalar::setto(const MonEntryScalar& o)
{
  params(o.desc());
  for(unsigned i=0; i<_desc.elements(); i++)
    _y[i]=o._y[i];
}

void MonEntryScalar::setvalues(const double* o)
{
  for(unsigned i=0; i<_desc.elements(); i++)
    _y[i]=o[i];
}
