#ifndef Pds_EbEvrSequence
#define Pds_EbEvrSequence

namespace Pds {

  class EbEvrSequence {
  public:
    EbEvrSequence() {}

    void* operator new(unsigned,void* p) { return p; }
    void  operator delete(void*) {}

    EbEvrSequence& operator=(const EbEvrSequence&);
    bool operator==(const EbEvrSequence&) const;
    bool operator>=(const EbEvrSequence&) const;
  private:
    unsigned _data;
  };

}


inline Pds::EbEvrSequence& Pds::EbEvrSequence::operator=(const Pds::EbEvrSequence& p)
{
  _data = p._data;
  return *this;
}

inline bool Pds::EbEvrSequence::operator==(const Pds::EbEvrSequence& p) const
{
  return (_data==p._data);
}

inline bool Pds::EbEvrSequence::operator>=(const Pds::EbEvrSequence& p) const
{
  return (_data>=p._data);
}

#endif
