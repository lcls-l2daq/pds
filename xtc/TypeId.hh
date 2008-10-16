#ifndef Pds_TypeId_hh
#define Pds_TypeId_hh

#include "pds/service/ODMGTypes.hh"

namespace Pds {

  class TypeNum {
  public:
    enum { Any = 0 };
    enum { Id_Xtc   = 0x000F0000 };
    enum { Id_InXtc = 0x000F0001 };
  };

  class TypeId {
  public:
    TypeId(unsigned v) : value(v) {}
    TypeId(const TypeId& v) : value(v.value) {}

    operator unsigned() const { return value; }
    
    d_ULong value;
  };

}

#endif
