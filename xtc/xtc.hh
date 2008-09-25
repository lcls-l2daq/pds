#ifndef XTC_HH
#define XTC_HH

#include "pds/service/ODMGTypes.hh"
#include "Src.hh"

namespace Pds {

class odfdAdr {
private:
  d_ULong _value;
};

class odfpAdr {
private:
  d_ULong _value;
};

class odfdTag {
private:
  d_ULong _tag;
};

class odfElementId  {
public:
  enum Value {element0,  element1,  element2,  element3,
	      element4,  element5,  element6,  element7,
	      element8,  element9,  element10, element11,
	      element12, element13, element14, element15,
	      element16, element17, element18, element19,
	      element20, element21, element22, element23,
	      element24, element25, element26, element27,
	      element28, element29, element30, element31};
  enum {numberof=32};
};

class odfElement {
public:
  unsigned            size() const;
  odfElementId::Value id()   const;
  odfElement*         next() const;

private:
  d_ULong _sizeAndId;     // Upper 16 bits: size, lower 16 bits: id
};

class odfDamage
{
public:
  enum Value {
    DroppedContribution    = 1,
    IncompleteContribution = 15
  };
  odfDamage(unsigned v) : _damage(v) {}
  d_ULong  value() const                { return _damage; }
  void     increase(odfDamage::Value v) { _damage |= (1<<v); }
  void     increase(d_ULong v)          { _damage |= v; }

private:
  d_ULong _damage;
};

class odfTypeNumPrimary {
public:
  enum { Any = 0 };
  enum { Id_unsigned_char = 0x000F0001,
	 Id_signed_char   = 0x000F0002,
	 Id_d_UShort      = 0x000F0005,
	 Id_d_Short       = 0x000F0006,
	 Id_d_ULong       = 0x000F0007,
	 Id_d_Long        = 0x000F0008,
	 Id_d_Float       = 0x000F0009,
	 Id_d_Double      = 0x000F000A,

	 Id_XTC        = 0x000F0082,
	 Id_TestXTC    = 0x000F0083,
	 Id_SimpleXTC  = 0x000F0084,
	 Id_Arena      = 0x000F0085,

	 // Identity and contains values for the event builder's use
	 Id_InXtcContainer    = 0x000F0091
  };
};

class odfTypeId {
public:
  odfTypeId(unsigned v) : value(v) {}
  odfTypeId(const odfTypeId& v) : value(v.value) {}

  operator unsigned() const { return value; }

  d_ULong value;
};

class odfTC {
public:
  odfTC( const odfTC& tc ) : _contains(tc._contains) {}
  odfTC( const odfTypeId& ctns ) : _contains(ctns) {}
  inline const odfTypeId& contains() const { return _contains; }

private:
  odfTypeId _contains;
};

class odfXTC : public odfTC {
public:
  odfXTC( ) : odfTC( odfTypeId( odfTypeNumPrimary::Any ) ),
	      _extent( sizeof(odfXTC) ) {}
  odfXTC( const odfTC& ctns ) : odfTC(ctns), 
				_extent( sizeof(odfXTC) ) {}
  
  inline void* operator new(unsigned,void* p) { return p; }
  inline void  operator delete(void*) {}

  inline void*  base() const { return (void*)this; }
  inline const void*  end() const { return (char*)this + _extent; }
  inline d_ULong  extent() const { return _extent; }
  inline d_ULong  extend( d_ULong extension ) { return _extent += extension; }
  inline char*  next() { return (char*) this + _extent; }
  inline void*  alloc( d_ULong size ) { void* buffer = next(); _extent += size; return buffer; }
  inline void   free( void* buffer ) {
    char* cp = (char*) buffer;
    if ( ( cp >= (char*)base() )  &&  ( cp < ( (char*)this + _extent ) ) ) {
      _extent = cp - (char*)this;
    }
  }

private:
  d_ULong   _extent;
};

class odfInXtc {
public:
  odfInXtc() : damage(0), tag() {}
  odfInXtc(const odfTC& tc) : damage(0), tag(tc) {}
  odfInXtc(const odfTC& tc, const Src& _src) : damage(0), src(_src), tag(tc)
{}
  odfInXtc(const odfTC& _tag, const Src& _src, unsigned& _damage) : damage(_damage), src(_src), tag(_tag) {}
  odfInXtc(const odfTC& _tag, const Src& _src, const odfDamage& _damage) : damage(_damage), src(_src), tag(_tag) {}

  void* operator new(unsigned size, odfXTC* xtc) { return (void*)xtc->alloc(size); }
  void* operator new(unsigned size, char* p)     { return (void*)p; }

  char*           payload()       const { return (char*)(this+1); }
  int             sizeofPayload() const { return tag.extent() - sizeof(odfXTC); }
  odfInXtc*       next()                { return (odfInXtc*) tag.next(); }
  const odfInXtc* next()          const { return (const odfInXtc*) tag.end(); }

  odfDamage damage;
  Src       src;
  odfXTC    tag;
};


//
//  For convenience, typedef the odf* classes without odf prefix
//

typedef odfdAdr               dAdr;
typedef odfpAdr               pAdr;
typedef odfdTag               dTag;
typedef odfElementId          ElementId;
typedef odfElement            Element;
typedef odfDamage             Damage;
typedef odfTypeNumPrimary     TypeNumPrimary;
typedef odfTypeId             TypeId;
typedef odfTC                 TC;
typedef odfXTC                XTC;
typedef odfInXtc              InXtc;

}

#endif
