#ifndef Pds_EbPulseId
#define Pds_EbPulseId

#include "pds/xtc/Sequence.hh"

namespace Pds {

  class EbPulseId {
  public:
    EbPulseId() { _data[0]=_data[1]=-1UL; }
    EbPulseId(const EbPulseId& p) { _data[0]=p._data[0]; _data[1]=p._data[1]; }
    EbPulseId(const Sequence&  s) { _data[0]=s.low();    _data[1]=s.high(); }

    void* operator new(unsigned,void* p) { return p; }
    void  operator delete(void*) {}

    EbPulseId& operator=(const EbPulseId&);
    bool operator==(const EbPulseId&) const;
    bool operator>=(const EbPulseId&) const;

    bool operator==(const Sequence&) const;
    bool operator>=(const Sequence&) const;

    //  private:
    unsigned _data[2];
  };

}


inline Pds::EbPulseId& Pds::EbPulseId::operator=(const Pds::EbPulseId& p)
{
  _data[0] = p._data[0];
  _data[1] = p._data[1];
  return *this;
}

inline bool Pds::EbPulseId::operator==(const Pds::EbPulseId& p) const
{
  return (_data[0]==p._data[0]) && (_data[1]==p._data[1]);
}

inline bool Pds::EbPulseId::operator>=(const Pds::EbPulseId& p) const
{
  return (_data[1]>p._data[1]) || ((_data[1]==p._data[1]) &&
				   (_data[0]>=p._data[0]));
}

inline bool Pds::EbPulseId::operator==(const Pds::Sequence& p) const
{
  return (_data[0]==p.low()) && (_data[1]==p.high());
}

inline bool Pds::EbPulseId::operator>=(const Pds::Sequence& p) const
{
  return (_data[1]>p.high()) || ((_data[1]==p.high()) &&
				 (_data[0]>=p.low ()));
}

#endif
