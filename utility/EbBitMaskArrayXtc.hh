#ifndef PDS_EBBITMASKARRAYXTC_HH
#define PDS_EBBITMASKARRAYXTC_HH

#include "OutletWireInsList.hh"
#include "pds/service/EbBitMaskArray.hh"
#include "pds/service/RingPool.hh"

namespace Pds {
class EbBitMaskArrayXtc : public InXtc {
public:
  EbBitMaskArrayXtc(const Src& src, OutletWireInsList& nodes) : 
    InXtc(EbBitMaskArrayTC(), src, Damage(0))
  {
    tag.extend(sizeof(EbBitMaskArray));

    if (!nodes.isempty()) {
      unsigned i=0;
      OutletWireIns* first = nodes.lookup(i);
      OutletWireIns* current = first;
      do {
	array.setBit(current->id());
	current = nodes.lookup(++i);
      } while (current != first);
    }
  }

  void* operator new(unsigned, Pool*);
  void  operator delete(void*);

  EbBitMaskArray array;

private:
  class EbBitMaskArrayTC : public TC {
  public:
    EbBitMaskArrayTC() :
      TC(TypeIdQualified(TypeNumPrimary::Any),
	    TypeIdQualified(TypeNumPrimary::Id_EbBitMaskArray)) {}
  };
};
}


inline void* Pds::EbBitMaskArrayXtc::operator new(unsigned size,Pds::Pool* pool)
{
  return pool->alloc(size);
}


inline void  Pds::EbBitMaskArrayXtc::operator delete(void* buffer)
{
  Pds::RingPool::free(buffer);
}


#endif
