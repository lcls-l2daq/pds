#include "pds/mon/MonStatsScalar.hh"
#include "pds/mon/MonDescScalar.hh"

#include <math.h>

using namespace Pds;

MonStatsScalar::MonStatsScalar(unsigned n, const double* v) : _n(n)
{
  double* y = reinterpret_cast<double*>(this+1);
  for(unsigned i=0; i<_n; i++)
    y[i]=v[i];
}

MonStatsScalar::MonStatsScalar(const std::vector<double>& v) : _n(v.size())
{
  double* y = reinterpret_cast<double*>(this+1);
  for(unsigned i=0; i<_n; i++)
    y[i]=v[i];
}

MonStatsScalar::MonStatsScalar(const MonStatsScalar& o) : _n(o._n)
{
  double* y = reinterpret_cast<double*>(this+1);
  const double* v = reinterpret_cast<const double*>(&o+1);
  for(unsigned i=0; i<_n; i++)
    y[i]=v[i];
}

MonStatsScalar::~MonStatsScalar() {}

void MonStatsScalar::reset() 
{
  double* y = reinterpret_cast<double*>(this+1);
  for(unsigned i=0; i<_n; i++)
    y[i]=0;
}
  
void MonStatsScalar::setto(const MonStatsScalar& curr,
			   const MonStatsScalar& prev)
{
  double* y = reinterpret_cast<double*>(this+1);
  const double* a = reinterpret_cast<const double*>(&curr+1);
  const double* b = reinterpret_cast<const double*>(&prev+1);
  _n = curr._n;
  for(unsigned i=0; i<curr._n; i++)
    y[i] = a[i]-b[i];
}

unsigned MonStatsScalar::size() const {
  return sizeof(*this)+_n*sizeof(double);
}

unsigned MonStatsScalar::size(const MonDesc& d)
{
  const MonDescScalar& ds = static_cast<const MonDescScalar&>(d);
  return sizeof(MonStatsScalar)+ds.elements()*sizeof(double);
}
