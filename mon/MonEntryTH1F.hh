#ifndef Pds_MonENTRYTH1F_HH
#define Pds_MonENTRYTH1F_HH

#include "pds/mon/MonEntry.hh"
#include "pds/mon/MonDescTH1F.hh"
#include "pds/mon/MonStats1D.hh"

namespace Pds {

  class MonEntryTH1F : public MonEntry, public MonStats1D {
  public:
    MonEntryTH1F(const char* name, const char* xtitle, const char* ytitle,
		 bool isnormalized=false);
    MonEntryTH1F(const MonDescTH1F& desc);

    virtual ~MonEntryTH1F();

    void params(unsigned nbins, float xlow, float xup);
    void params(const MonDescTH1F& desc);

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

    void setto(const MonEntryTH1F& entry);
    void setto(const MonEntryTH1F& curr, const MonEntryTH1F& prev);
    void stats();

    // Implements MonEntry
    virtual const MonDescTH1F& desc() const;
    virtual MonDescTH1F& desc();

    void dump() const;
  private:
    void build(unsigned nbins);

  private:
    MonDescTH1F _desc;

  private:
    double* _y;
  };

  inline double MonEntryTH1F::content   (unsigned bin) const { return *(_y+bin); }
  inline void   MonEntryTH1F::addcontent(double y, unsigned bin) { *(_y+bin) += y; }
  inline void   MonEntryTH1F::content   (double y, unsigned bin) { *(_y+bin)  = y; }

  inline double MonEntryTH1F::info   (Info i) const   {return *(_y+_desc.nbins()+int(i));}
  inline void   MonEntryTH1F::info   (double y, Info i) {*(_y+_desc.nbins()+int(i)) = y;}
  inline void   MonEntryTH1F::addinfo(double y, Info i) {*(_y+_desc.nbins()+int(i)) += y;}
};

#endif
