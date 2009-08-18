#ifndef Pds_MonENTRYWaveform_HH
#define Pds_MonENTRYWaveform_HH

#include "pds/mon/MonEntry.hh"
#include "pds/mon/MonDescWaveform.hh"
#include "pds/mon/MonStats1D.hh"

namespace Pds {

  class MonEntryWaveform : public MonEntry, public MonStats1D {
  public:
    MonEntryWaveform(const char* name, const char* xtitle, const char* ytitle,
		     bool isnormalized=false);
    MonEntryWaveform(const MonDescWaveform& desc);

    virtual ~MonEntryWaveform();

    void params(unsigned nbins, float xlow, float xup);
    void params(const MonDescWaveform& desc);

    //  The contents are organized as 
    //  [ data0, data1, ..., dataN-1, underflow, overflow, normalization ]
    double content(unsigned bin) const;
    void   addcontent(double y, unsigned bin);
    void   content(double y, unsigned bin);
    void   addcontent(double y, double x);

    enum Info { Underflow, Overflow, Normalization, InfoSize };
    double info(Info) const;
    void   info(double y, Info);
    void   addinfo(double y, Info);

    void setto(const MonEntryWaveform& entry);
    void setto(const MonEntryWaveform& curr, const MonEntryWaveform& prev);
    void stats();

    // Implements MonEntry
    virtual const MonDescWaveform& desc() const;
    virtual MonDescWaveform& desc();

  private:
    void build(unsigned nbins);

  private:
    MonDescWaveform _desc;

  private:
    double* _y;
  };

  inline double MonEntryWaveform::content   (unsigned bin) const { return *(_y+bin); }
  inline void   MonEntryWaveform::addcontent(double y, unsigned bin) { *(_y+bin) += y; }
  inline void   MonEntryWaveform::content   (double y, unsigned bin) { *(_y+bin)  = y; }

  inline double MonEntryWaveform::info   (Info i) const   {return *(_y+_desc.nbins()+int(i));}
  inline void   MonEntryWaveform::info   (double y, Info i) {*(_y+_desc.nbins()+int(i)) = y;}
  inline void   MonEntryWaveform::addinfo(double y, Info i) {*(_y+_desc.nbins()+int(i)) += y;}
};

#endif
