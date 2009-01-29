#include "TwoDMoments.hh"

using namespace Pds;

TwoDMoments::TwoDMoments(unsigned cols,
			 unsigned rows,
			 const unsigned short* src) :
  _n(0), _x(0), _y(0), _xx(0), _yy(0), _xy(0)
{
  for(unsigned k=0; k<rows; k++) {
    unsigned wsum  = 0;
    unsigned wxsum = 0;
    for(unsigned j=0; j<cols; j++) {
      unsigned short d  = *src++;
      unsigned long  dj = d*j;
      wsum  += d;
      wxsum += dj;
      _xx   += dj*j;
    }
    _n  += wsum;
    _x  += wxsum;
    _y  += wsum*k;
    _yy += wsum*k*k;
    _xy += wxsum*k;
  }
}

TwoDMoments::TwoDMoments(unsigned cols,
			 unsigned colStart, unsigned colEnd,
			 unsigned rowStart, unsigned rowEnd,
			 const unsigned short* src) :
  _n(0), _x(0), _y(0), _xx(0), _yy(0), _xy(0)
{
  src += rowStart*cols;
  for(unsigned k=rowStart; k<rowEnd; k++) {
    unsigned wsum  = 0;
    unsigned wxsum = 0;
    src += colStart;
    for(unsigned j=colStart; j<colEnd; j++) {
      unsigned short d  = *src++;
      unsigned long  dj = d*j;
      wsum  += d;
      wxsum += dj;
      _xx   += dj*j;
    }
    src += cols-colEnd;
    _n  += wsum;
    _x  += wxsum;
    _y  += wsum*k;
    _yy += wsum*k*k;
    _xy += wxsum*k;
  }
}

void TwoDMoments::accumulate(unsigned long long column, 
			     unsigned long long row, 
			     unsigned long long w) {
  _n += w;
  unsigned long long x = w*column;
  unsigned long long y = w*row;
  _x += x;
  _y += y;
  _xx += x*column;
  _yy += y*row;
  _xy += x*row;
}
