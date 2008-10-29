#ifndef Pds_Xtc_hh
#define Pds_Xtc_hh

#include "TypeId.hh"
#include "Damage.hh"
#include "Src.hh"

namespace Pds {

  class Xtc {
  public:
    Xtc() : 
      damage(0), contains(TypeNum::Any), extent(sizeof(Xtc)) {}
    Xtc(const Xtc& xtc) :
      damage(xtc.damage), src(xtc.src), contains(xtc.contains), extent(sizeof(Xtc)) {}
    Xtc(const TypeId& type) : 
      damage(0), contains(type), extent(sizeof(Xtc)) {}
    Xtc(const TypeId& type, const Src& _src) : 
      damage(0), src(_src), contains(type), extent(sizeof(Xtc)) {}
    Xtc(const TypeId& _tag, const Src& _src, unsigned _damage) : 
      damage(_damage), src(_src), contains(_tag), extent(sizeof(Xtc)) {}
    Xtc(const TypeId& _tag, const Src& _src, const Damage& _damage) : damage(_damage), src(_src), contains(_tag), extent(sizeof(Xtc)) {}
    
    void* operator new(unsigned size, char* p)     { return (void*)p; }
    void* operator new(unsigned size, Xtc* p)    { return p->alloc(size); }
    
    char*        payload()       const { return (char*)(this+1); }
    int          sizeofPayload() const { return extent - sizeof(Xtc); }
    Xtc*       next()                { return (Xtc*)((char*)this+extent); }
    const Xtc* next()          const { return (const Xtc*)((char*)this+extent); }
    
    void*        alloc(d_ULong size )  { void* buffer = next(); extent += size; return buffer; }

    Damage    damage;
    Src       src;
    TypeId    contains;
    unsigned  extent;
  };
}


#endif
