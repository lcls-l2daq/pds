#ifndef Pds_MonENTRYImage_HH
#define Pds_MonENTRYImage_HH

#include "pds/mon/MonEntry.hh"
#include "pds/mon/MonDescImage.hh"

namespace Pds {

  class MonEntryImage : public MonEntry {
  public:
    MonEntryImage(const char* name);
    MonEntryImage(const MonDescImage& desc);

    virtual ~MonEntryImage();

    void params(unsigned nbinsx,
		unsigned nbinsy,
		int ppxbin,
		int ppybin);
    void params(const MonDescImage& desc);

    unsigned content   (unsigned bin) const;
    unsigned content   (unsigned binx, unsigned biny) const;
    void     addcontent(unsigned y, unsigned binx, unsigned biny);
    void     content   (unsigned y, unsigned binx, unsigned biny);

    unsigned*       contents();
    const unsigned* contents() const;

    enum Info { Pedestal, Normalization, InfoSize };
    unsigned info   (Info) const;
    void     info   (unsigned y, Info);
    void     addinfo(unsigned y, Info);

    void setto(const MonEntryImage& entry);
    void setto(const MonEntryImage& curr, const MonEntryImage& prev);

    // Implements MonEntry
    virtual const MonDescImage& desc() const;
    virtual MonDescImage& desc();

  private:
    void build(unsigned nbinsx, unsigned nbinsy);

  private:
    MonDescImage _desc;

  private:
    unsigned* _y;
  };

  inline unsigned MonEntryImage::content(unsigned bin) const 
  {
    return *(_y+bin); 
  }
  inline unsigned MonEntryImage::content(unsigned binx, unsigned biny) const 
  {
    return *(_y+binx+biny*_desc.nbinsx()); 
  }
  inline void MonEntryImage::addcontent(unsigned y, unsigned binx, unsigned biny) 
  {
    *(_y+binx+biny*_desc.nbinsx()) += y;
  }
  inline void MonEntryImage::content(unsigned y, unsigned binx, unsigned biny) 
  {
    *(_y+binx+biny*_desc.nbinsx()) = y;
  }
  inline unsigned* MonEntryImage::contents()
  {
    return _y;
  }
  inline const unsigned* MonEntryImage::contents() const
  {
    return _y;
  }
  inline unsigned MonEntryImage::info(Info i) const 
  {
    return *(_y+_desc.nbinsx()*_desc.nbinsy()+int(i));
  }
  inline void MonEntryImage::info(unsigned y, Info i) 
  {
    *(_y+_desc.nbinsx()*_desc.nbinsy()+int(i)) = y;
  }
  inline void MonEntryImage::addinfo(unsigned y, Info i) 
  {
    *(_y+_desc.nbinsx()*_desc.nbinsy()+int(i)) += y;
  }
};

#endif
