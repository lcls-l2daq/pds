#ifndef Pds_MonENTRYDESCTH1F_HH
#define Pds_MonENTRYDESCTH1F_HH

#include "pds/mon/MonDescEntry.hh"

namespace Pds {

  class MonDescTH1F : public MonDescEntry {
  public:
    MonDescTH1F(const char* name, 
		const char* xtitle, 
		const char* ytitle, 
		unsigned nbins, 
		float xlow, 
		float xup,
		bool isnormalized=false);

    void params(unsigned nbins, float xlow, float xup);

    unsigned nbins() const;
    float xlow() const;
    float xup() const;

  private:
    unsigned short _nbins;
    unsigned short _unused;
    float _xlow;
    float _xup;
  };

  inline unsigned MonDescTH1F::nbins() const {return _nbins;}
  inline float MonDescTH1F::xlow() const {return _xlow;}
  inline float MonDescTH1F::xup() const {return _xup;}
};

#endif
