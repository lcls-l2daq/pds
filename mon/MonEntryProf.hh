#ifndef Pds_MonENTRYPROF_HH
#define Pds_MonENTRYPROF_HH

#include <math.h>

#include "pds/mon/MonEntry.hh"
#include "pds/mon/MonDescProf.hh"

namespace Pds {

  class MonEntryProf : public MonEntry {
  public:
    MonEntryProf(const char* name, const char* xtitle, const char* ytitle);
    MonEntryProf(const MonDescProf& desc);

    virtual ~MonEntryProf();

    void params(unsigned nbins, float xlow, float xup, const char* names);
    void params(const MonDescProf& desc);

    double ymean(unsigned bin) const;
    double sigma(unsigned bin) const;

    double ysum(unsigned bin) const;
    void ysum(double y, unsigned bin);

    double y2sum(unsigned bin) const;
    void y2sum(double y2, unsigned bin);

    double nentries(unsigned bin) const;
    void nentries(double nent, unsigned bin);

    void addy(double y, unsigned bin);

    enum Info { Underflow, Overflow, InfoSize };
    double info(Info) const;
    void   info(double, Info);
    void   addinfo(double, Info);

    void setto(const MonEntryProf& entry);
    void setto(const MonEntryProf& curr, const MonEntryProf& prev);

    // Implements MonEntry
    virtual const MonDescProf& desc() const;
    virtual MonDescProf& desc();

  private:
    void build(unsigned nbins);

  private:
    MonDescProf _desc;

  private:
    double* _ysum;
    double* _y2sum;
    double* _nentries;
  };

  inline double MonEntryProf::ymean(unsigned bin) const 
  {
    double n = *(_nentries+bin);
    if (n > 0) {
      double y = *(_ysum+bin);
      double mean = y/n;
      return mean;
    } else {
      return 0;
    }
  }

  inline double MonEntryProf::sigma(unsigned bin) const 
  {
    double n = *(_nentries+bin);
    if (n > 0) {
      double y = *(_ysum+bin);
      double y2 = *(_y2sum+bin);
      double mean = y/n;
      double s = sqrt(y2/n-mean*mean);
      return s;
    } else {
      return 0;
    }
  }

  inline double MonEntryProf::ysum(unsigned bin) const {return *(_ysum+bin);}
  inline void MonEntryProf::ysum(double y, unsigned bin) {*(_ysum+bin) = y;}

  inline double MonEntryProf::y2sum(unsigned bin) const {return *(_y2sum+bin);}
  inline void MonEntryProf::y2sum(double y2, unsigned bin) {*(_y2sum+bin) = y2;}

  inline double MonEntryProf::nentries(unsigned bin) const {return *(_nentries+bin);}
  inline void MonEntryProf::nentries(double nent, unsigned bin) {*(_nentries+bin) = nent;}

  inline void MonEntryProf::addy(double y, unsigned bin) 
  {
    _ysum[bin] += y;
    _y2sum[bin] += y*y;
    _nentries[bin] += 1;
  }

  inline double MonEntryProf::info(Info i) const { return *(_nentries+_desc.nbins()+int(i)); }
  inline void   MonEntryProf::info(double y,Info i) { *(_nentries+_desc.nbins()+int(i)) = y; }
  inline void   MonEntryProf::addinfo(double y,Info i) { *(_nentries+_desc.nbins()+int(i)) += y; }
};

#endif
