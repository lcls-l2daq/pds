#include "pds/mon/MonStats2D.hh"

#include <math.h>

using namespace Pds;

static double integral(unsigned binx1,  
		       unsigned binx2,
		       unsigned biny1,  
		       unsigned biny2,
		       unsigned nbinsx,
		       const float* con)
{
  double sum=0;
  nbinsx+=2;
  for (unsigned biny=biny1; biny<=biny2; biny++) {
    for (unsigned binx=binx1; binx<=binx2; binx++) {
      unsigned bin = binx + nbinsx*biny;
      sum += con[bin];
    }
  }
  return sum;
}

MonStats2D::MonStats2D() {}
MonStats2D::~MonStats2D() {}

void MonStats2D::stats(unsigned nbinsx, 
		       unsigned nbinsy, 
		       float xlo, float xup, 
		       float ylo, float yup, 
		       const float* con)
{
  unsigned nx = nbinsx;
  unsigned ny = nbinsy;

  _sumw   = 0; 
  _sumw2  = 0; 
  _sumwx  = 0; 
  _sumwx2 = 0;
  _sumwy  = 0; 
  _sumwy2 = 0;
  _sumwxy = 0;

  float dx = (xup-xlo)/nbinsx;
  float dy = (yup-ylo)/nbinsy;
  float y = ylo+dy*0.5;
  for (unsigned biny=0; biny<ny; biny++) {
    float x = xlo+dx*0.5;
    double sw=0, swx=0;
    for (unsigned binx=0; binx<nx; binx++) {
      double w = *con++;
      sw      += w;
      _sumw2  += w*w;
      swx     += w*x;
      _sumwx2 += w*x*x;
      x += dx;
    }
    _sumw   += sw;
    _sumwx  += swx;
    _sumwy  += sw*y;
    _sumwy2 += sw*y*y;
    _sumwxy += swx*y;
    y += dy;
  }
}

double MonStats2D::meanx() const
{
  return _sumw ? _sumwx/_sumw : 0;
}

double MonStats2D::rmsx() const
{
  if (_sumw) {
    double mean = _sumwx/_sumw;
    return sqrt(_sumwx2/_sumw-mean*mean);
  } else {
    return 0;
  }
}

double MonStats2D::meany() const
{
  return _sumw ? _sumwy/_sumw : 0;
}

double MonStats2D::rmsy() const
{
  if (_sumw) {
    double mean = _sumwy/_sumw;
    return sqrt(_sumwy2/_sumw-mean*mean);
  } else {
    return 0;
  }
}

double MonStats2D::sum  () const { return _sumw; }
double MonStats2D::sumx () const { return _sumwx; }
double MonStats2D::sumy () const { return _sumwy; }
double MonStats2D::sumx2() const { return _sumwx2; }
double MonStats2D::sumy2() const { return _sumwy2; }
double MonStats2D::sumxy() const { return _sumwxy; }
