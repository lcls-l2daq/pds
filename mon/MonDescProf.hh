#ifndef Pds_MonDESCPROF_HH
#define Pds_MonDESCPROF_HH

#include "pds/mon/MonDescEntry.hh"

namespace Pds {

  class MonDescProf : public MonDescEntry {
  public:
    MonDescProf(const char* name, const char* xtitle, const char* ytitle, 
		unsigned nbins, float xlow, float xup, const char* names);

    unsigned nbins() const;
    float xlow() const;
    float xup() const;
    const char* names() const;  

    void params(unsigned nbins, float xlow, float xup, const char* names);

  private:
    enum {NamesSize=256};
    char _names[NamesSize];
    unsigned _nbins;
    unsigned _unused;
    float _xlow;
    float _xup;
  };

  inline unsigned MonDescProf::nbins() const {return _nbins;}
  inline float MonDescProf::xlow() const {return _xlow;}
  inline float MonDescProf::xup() const {return _xup;}
  inline const char* MonDescProf::names() const {return _names;}
};

#endif
