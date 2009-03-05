#ifndef Pds_TwoDMoments_hh
#define Pds_TwoDMoments_hh

namespace Pds {

  class TwoDMoments {
  public:
    TwoDMoments() : _n(0), _x(0), _y(0), _xx(0), _yy(0), _xy(0) {}
    TwoDMoments(unsigned long long w,
		unsigned long long wx,
		unsigned long long wy,
		unsigned long long wxx,
		unsigned long long wyy,
		unsigned long long wxy) : 
      _n(w), _x(wx), _y(wy), _xx(wxx), _yy(wyy), _xy(wxy) {}
    TwoDMoments(unsigned cols, unsigned rows,
		unsigned short offset,
		const unsigned short* src);
    TwoDMoments(unsigned cols,
		unsigned colStart, unsigned colEnd,
		unsigned rowStart, unsigned rowEnd, 
		unsigned short offset,
		const unsigned short* src);
    TwoDMoments(unsigned cols,
		unsigned rows,
		unsigned short offset,
		unsigned short threshold,
		const unsigned short* src);
    ~TwoDMoments() {}

  public:
    unsigned long long _n;
    unsigned long long _x;
    unsigned long long _y;
    unsigned long long _xx;
    unsigned long long _yy;
    unsigned long long _xy;
  };
};

#endif

