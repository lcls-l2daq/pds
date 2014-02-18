#include "pds/mon/MonStats1D.hh"

#include <math.h>

using namespace Pds;

MonStats1D::MonStats1D() {}
MonStats1D::~MonStats1D() {}

void MonStats1D::reset() 
{
  _sumw   = 0; 
  _sumw2  = 0; 
  _sumwx  = 0; 
  _sumwx2 = 0;
  _under  = 0;
  _over   = 0;
}
  
void MonStats1D::stats(unsigned nbins, 
		       float xlo, float xup, 
		       const double* con)
{
  _sumw   = 0; 
  _sumw2  = 0; 
  _sumwx  = 0; 
  _sumwx2 = 0;
  _under  = *(con+nbins);
  _over   = *(con+nbins+1);
  const double* end = con+nbins;
  float dx = (xup-xlo)/nbins;
  float x  = xlo+0.5*dx;
  do {
    double w = *con++;
    _sumw   += w;
    _sumw2  += w*w;
    _sumwx  += w*x;
    _sumwx2 += w*x*x;
    x += dx;
  } while (con < end);

  x = xlo - 0.5*dx;
  double w = _under;
  _sumw   += w;
  _sumw2  += w*w;
  _sumwx  += w*x;
  _sumwx2 += w*x*x; 

  x = xup + 0.5*dx;
  w = _over;
  _sumw   += w;
  _sumw2  += w*w;
  _sumwx  += w*x;
  _sumwx2 += w*x*x; 
}

void MonStats1D::setto(const MonStats1D& s)
{
  _sumw   = s._sumw  ;
  _sumw2  = s._sumw2 ;
  _sumwx  = s._sumwx ;
  _sumwx2 = s._sumwx2;
}

void MonStats1D::setto(const MonStats1D& curr,
		       const MonStats1D& prev)
{
  _sumw   = curr._sumw   - prev._sumw  ;
  _sumw2  = curr._sumw2  - prev._sumw2 ;
  _sumwx  = curr._sumwx  - prev._sumwx ;
  _sumwx2 = curr._sumwx2 - prev._sumwx2;
}

double MonStats1D::sum() const 
{
  return _sumw;
}

double MonStats1D::sumx() const { return _sumwx; }

double MonStats1D::sumx2() const { return _sumwx2; }

double MonStats1D::mean() const
{
  return _sumw ? _sumwx/_sumw : 0;
}
 
double MonStats1D::rms() const
{
  if (_sumw) {
    double mean = _sumwx/_sumw;
    return sqrt(_sumwx2/_sumw-mean*mean);
  } else {
    return 0;
  }
}

double MonStats1D::under() const 
{
  return _under;
}

double MonStats1D::over() const
{
  return _over;
}

void MonStats1D::reset()
{
  _sumw   = 0; 
  _sumw2  = 0; 
  _sumwx  = 0; 
  _sumwx2 = 0;
  _under  = 0;
  _over   = 0;
}
