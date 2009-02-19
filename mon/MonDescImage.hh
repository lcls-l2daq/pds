#ifndef Pds_MonDescImage_HH
#define Pds_MonDescImage_HH

#include "pds/mon/MonDescEntry.hh"

namespace Pds {

  class MonDescImage : public MonDescEntry {
  public:
    MonDescImage(const char* name,
		 unsigned nbinsx, unsigned nbinsy, 
		 int ppbx=1, int ppby=1); // pixels per bin

    unsigned nbinsx() const;
    unsigned nbinsy() const;
    int ppxbin() const;
    int ppybin() const;
    float xlow() const;
    float xup() const;
    float ylow() const;
    float yup() const;

    void params(unsigned nbinsx,
		unsigned nbinsy,
		int ppxbin,
		int ppybin);

  private:
    unsigned short _nbinsx;
    unsigned short _nbinsy;
    unsigned short _ppbx;
    unsigned short _ppby;
  };

  inline unsigned MonDescImage::nbinsx() const {return _nbinsx;}
  inline unsigned MonDescImage::nbinsy() const {return _nbinsy;}
  inline int MonDescImage::ppxbin() const {return _ppbx;}
  inline int MonDescImage::ppybin() const {return _ppby;}
  inline float MonDescImage::xlow() const {return 0;}
  inline float MonDescImage::xup() const {return _nbinsx*_ppbx;}
  inline float MonDescImage::ylow() const {return 0;}
  inline float MonDescImage::yup() const {return _nbinsy*_ppby;}
};

#endif
