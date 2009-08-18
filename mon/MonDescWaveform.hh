#ifndef Pds_MonENTRYDESCWaveform_HH
#define Pds_MonENTRYDESCWaveform_HH

#include "pds/mon/MonDescEntry.hh"

namespace Pds {

  class MonDescWaveform : public MonDescEntry {
  public:
    MonDescWaveform(const char* name, 
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

  inline unsigned MonDescWaveform::nbins() const {return _nbins;}
  inline float MonDescWaveform::xlow() const {return _xlow;}
  inline float MonDescWaveform::xup() const {return _xup;}
};

#endif
