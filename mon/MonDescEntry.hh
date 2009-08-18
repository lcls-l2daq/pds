#ifndef Pds_MonDESCENTRY_HH
#define Pds_MonDESCENTRY_HH

#include "pds/mon/MonDesc.hh"

namespace Pds {

  class MonDescEntry : public MonDesc {
  public:
    enum Type {TH1F, TH2F, Prof, Image, Waveform};
    Type type() const;

    const char* xtitle() const;
    const char* ytitle() const;
    int signature() const;
    int short group() const;
    unsigned short size() const;

    bool isnormalized() const;

    void xwarnings(float warn, float err);
    bool xhaswarnings() const;
    float xwarn() const;
    float xerr() const;

    void ywarnings(float warn, float err);
    bool yhaswarnings() const;
    float ywarn() const;
    float yerr() const;

  private:
    friend class MonGroup;
    void group(int short g);

  protected:
    MonDescEntry(const char* name, const char* xtitle, const char* ytitle, 
		 Type type, unsigned short size, bool isnormalized=false);

  private:
    enum {TitleSize=128};
    char _xtitle[TitleSize];
    char _ytitle[TitleSize];
    int short _group;
    int short _options;
    unsigned short _type;
    unsigned short _size;
    float _xwarn;
    float _xerr;
    float _ywarn;
    float _yerr;

  private:
    enum Option {Normalized, XWarnings, YWarnings};
  };
};

#endif
