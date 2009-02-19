#ifndef Pds_MonENTRYDESCTH2F_HH
#define Pds_MonENTRYDESCTH2F_HH

#include "pds/mon/MonDescEntry.hh"

namespace Pds {

  class MonDescTH2F : public MonDescEntry {
  public:
    MonDescTH2F(const char* name, const char* xtitle, const char* ytitle, 
		unsigned nbinsx, float xlow, float xup,
		unsigned nbinsy, float ylow, float yup,
		bool isnormalized=false);

    unsigned nbinsx() const;
    unsigned nbinsy() const;
    float xlow() const;
    float xup() const;
    float ylow() const;
    float yup() const;

    void params(unsigned nbins, float xlow, float xup,
		unsigned nbinsy, float ylow, float yup);

  private:
    unsigned short _nbinsx;
    unsigned short _nbinsy;
    float _xlow;
    float _xup;
    float _ylow;
    float _yup;
  };

  inline unsigned MonDescTH2F::nbinsx() const {return _nbinsx;}
  inline unsigned MonDescTH2F::nbinsy() const {return _nbinsy;}
  inline float MonDescTH2F::xlow() const {return _xlow;}
  inline float MonDescTH2F::xup() const {return _xup;}
  inline float MonDescTH2F::ylow() const {return _ylow;}
  inline float MonDescTH2F::yup() const {return _yup;}
};

#endif
