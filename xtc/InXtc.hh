#ifndef Pds_InXtc_hh
#define Pds_InXtc_hh

#include "TypeId.hh"
#include "Damage.hh"
#include "Src.hh"

namespace Pds {

  class InXtc {
  public:
    InXtc() : 
      damage(0), contains(TypeNum::Any), extent(sizeof(InXtc)) {}
    InXtc(const InXtc& xtc) :
      damage(xtc.damage), src(xtc.src), contains(xtc.contains), extent(sizeof(InXtc)) {}
    InXtc(const TypeId& type) : 
      damage(0), contains(type), extent(sizeof(InXtc)) {}
    InXtc(const TypeId& type, const Src& _src) : 
      damage(0), src(_src), contains(type), extent(sizeof(InXtc)) {}
    InXtc(const TypeId& _tag, const Src& _src, unsigned _damage) : 
      damage(_damage), src(_src), contains(_tag), extent(sizeof(InXtc)) {}
    InXtc(const TypeId& _tag, const Src& _src, const Damage& _damage) : damage(_damage), src(_src), contains(_tag), extent(sizeof(InXtc)) {}
    
    void* operator new(unsigned size, char* p)     { return (void*)p; }
    void* operator new(unsigned size, InXtc* p)    { return p->alloc(size); }
    
    char*        payload()       const { return (char*)(this+1); }
    int          sizeofPayload() const { return extent - sizeof(InXtc); }
    InXtc*       next()                { return (InXtc*)((char*)this+extent); }
    const InXtc* next()          const { return (const InXtc*)((char*)this+extent); }
    
    void*        alloc(d_ULong size )  { void* buffer = next(); extent += size; return buffer; }

    Damage    damage;
    Src       src;
    TypeId    contains;
    unsigned  extent;
  };
}


#endif
