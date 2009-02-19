#ifndef Pds_MonENTRYTH2F_HH
#define Pds_MonENTRYTH2F_HH

#include "pds/mon/MonEntry.hh"
#include "pds/mon/MonDescTH2F.hh"
#include "pds/mon/MonStats2D.hh"

namespace Pds {

  class MonEntryTH2F : public MonEntry, public MonStats2D {
  public:
    MonEntryTH2F(const char* name, const char* xtitle, const char* ytitle,
		 bool isnormalized=false);
    MonEntryTH2F(const MonDescTH2F& desc);

    virtual ~MonEntryTH2F();

    void params(unsigned nbinsx, float xlow, float xup,
		unsigned nbinsy, float ylow, float yup);
    void params(const MonDescTH2F& desc);

    float content   (unsigned bin) const;
    float content   (unsigned binx, unsigned biny) const;
    void  addcontent(float y, unsigned binx, unsigned biny);
    void  content   (float y, unsigned binx, unsigned biny);

    enum Info { UnderflowX, OverflowX, UnderflowY, OverflowY, Normalization, InfoSize };
    float info   (Info) const;
    void  info   (float y, Info);
    void  addinfo(float y, Info);

    void setto(const MonEntryTH2F& entry);
    void setto(const MonEntryTH2F& curr, const MonEntryTH2F& prev);
    void stats();

    // Implements MonEntry
    virtual const MonDescTH2F& desc() const;
    virtual MonDescTH2F& desc();

  private:
    void build(unsigned nbinsx, unsigned nbinsy);

  private:
    MonDescTH2F _desc;

  private:
    float* _y;
  };

  inline float MonEntryTH2F::content(unsigned bin) const 
  {
    return *(_y+bin); 
  }
  inline float MonEntryTH2F::content(unsigned binx, unsigned biny) const 
  {
    return *(_y+binx+biny*_desc.nbinsx()); 
  }
  inline void MonEntryTH2F::addcontent(float y, unsigned binx, unsigned biny) 
  {
    *(_y+binx+biny*_desc.nbinsx()) += y;
  }
  inline void MonEntryTH2F::content(float y, unsigned binx, unsigned biny) 
  {
    *(_y+binx+biny*_desc.nbinsx()) = y;
  }

  inline float MonEntryTH2F::info(Info i) const 
  {
    return *(_y+_desc.nbinsx()*_desc.nbinsy()+int(i));
  }
  inline void MonEntryTH2F::info(float y, Info i) 
  {
    *(_y+_desc.nbinsx()*_desc.nbinsy()+int(i)) = y;
  }
  inline void MonEntryTH2F::addinfo(float y, Info i) 
  {
    *(_y+_desc.nbinsx()*_desc.nbinsy()+int(i)) += y;
  }
};

#endif
