#ifndef XTC_HH
#define XTC_HH

#include "pds/service/ODMGTypes.hh"

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

class odfSrc
{
public:
  odfSrc(unsigned v) : _id(v) {}
  odfSrc(unsigned pid, unsigned did) : _id((did<<16) | pid) {}
  d_ULong did()   const { return _id>>16; }
  d_ULong pid()   const { return _id&0xffff; }

  bool operator==(const odfSrc& s) const { return _id==s._id; }
private:
  d_ULong _id;
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

	 Id_TC         = 0x000F0081,
	 Id_XTC        = 0x000F0082,
	 Id_TestXTC    = 0x000F0083,
	 Id_SimpleXTC  = 0x000F0084,
	 Id_Arena      = 0x000F0085,

	 // Identity and contains values for the event builder's use
	 Id_EventContainer    = 0x000F0091,
	 Id_FragmentContainer = 0x000F0092,
	 Id_ModuleContainer   = 0x000F0093,
	 Id_ModuleBatch       = 0x000F0094,
	 Id_TransientBatch    = 0x000F0095,
	 Id_FragmentBatch     = 0x000F0096,
	 Id_EbBitMaskArray    = 0x000F009C,
	 Id_GhostContainer    = 0x000F009D,
	 Id_ElectorXTC        = 0x000F009E,
	 Id_RealignXTC        = 0x000F009F
  };
};

typedef d_ULong odfTypeIdPrimary_t;
typedef d_ULong odfTypeIdSecondary_t;
typedef d_ULong odfTypeIdValue_t;

class odfTypeId {
public:
  odfTypeIdPrimary_t getPrimary()   const;
  odfTypeIdSecondary_t getSecondary()   const;
  enum { odfTypeIdNull = 0 };
protected:
  odfTypeId( ) : _val( odfTypeIdNull ) { }
  odfTypeId( odfTypeIdValue_t value ) : _val(value) { }

  int operator==( const odfTypeId& id ) const
    { return _val == id._val; }
  int operator!=( const odfTypeId& id ) const
    { return _val != id._val; }

  enum { odfTypeIdPrimOff   = 0,
	 odfTypeIdPrimMax   = 0x000FFFFF,
	 odfTypeIdPrimMsk   = odfTypeIdPrimMax };
  enum { odfTypeIdSecyOff   = 20,
	 odfTypeIdSecyMax   = 0x00000FFF,
	 odfTypeIdSecyMsk   = odfTypeIdSecyMax << odfTypeIdSecyOff };

  int setPrimary( odfTypeIdPrimary_t prim ) {
    if ( prim <= (odfTypeIdValue_t) odfTypeIdPrimMax ) {
      _val &= ~((odfTypeIdValue_t) odfTypeIdPrimMsk);
      _val |= ( ( prim & odfTypeIdPrimMax ) << odfTypeIdPrimOff ); 
      return 1; }
    else {
      return 0; }
  }
      
  int setSecondary( odfTypeIdSecondary_t secy ) {
    if ( secy <= odfTypeIdSecyMax ) {
      _val &= ~((odfTypeIdValue_t) odfTypeIdSecyMsk);
      _val |= ( ( secy & odfTypeIdSecyMax ) << odfTypeIdSecyOff );
      return 1; }
    else {
      return 0; }
  }

private:
  odfTypeIdValue_t _val;
};


class odfTypeIdQualified : private odfTypeId {
public:
  odfTypeIdQualified( odfTypeIdValue_t value ) : odfTypeId( value ) { }
  odfTypeIdQualified( odfTypeIdPrimary_t   prim,   
		      odfTypeIdSecondary_t secy  ) : odfTypeId() { setPrimary( prim ); setSecondary( secy ); }
  int isA( const odfTypeIdQualified& tq ) const;
  int isA( const odfTypeIdPrimary_t& tp ) const;
  odfTypeIdSecondary_t secondary() const;
  int operator==( const odfTypeIdQualified& tq ) const
    { return odfTypeId::operator==( tq ); }
  int operator!=( const odfTypeIdQualified& tq ) const
    { return odfTypeId::operator!=( tq ); }
};


class odfTC {
public:
  odfTC( const odfTypeIdQualified& ctns, const odfTypeIdQualified& id ) : _contains(ctns), _identity(id) {}
  inline const odfTypeIdQualified& contains() const { return _contains; }
  inline const odfTypeIdQualified& identity() const { return _identity; }

private:
  odfTypeIdQualified _contains;
  odfTypeIdQualified _identity;
};

class odfXTC : public odfTC {
public:
  odfXTC( ) : odfTC( odfTypeIdQualified( odfTypeNumPrimary::Any ),
		     odfTypeIdQualified( odfTypeNumPrimary::Id_XTC ) ),
	      _extent( sizeof(odfXTC) ) {}
  odfXTC( const odfTC& tc ) : odfTC(tc), _extent( sizeof(odfXTC) ) {}
  
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
  d_ULong _extent;
};

class odfInXtc {
public:
  odfInXtc() : damage(0), src(0), tag() {}
  odfInXtc(const odfTC& tc) : damage(0), src(0), tag(tc) {}
  odfInXtc(const odfTC& tc, const odfSrc& _src) : damage(0), src(_src), tag(tc) {}
  odfInXtc(const odfTC& _tag, const odfSrc& _src, unsigned& _damage) : damage(_damage), src(_src), tag(_tag) {}
  odfInXtc(const odfTC& _tag, const odfSrc& _src, const odfDamage& _damage) : damage(_damage), src(_src), tag(_tag) {}

  void* operator new(unsigned size, odfXTC* xtc) { return (void*)xtc->alloc(size); }
  void* operator new(unsigned size, char* p)     { return (void*)p; }

  char*           payload()       const { return (char*)(this+1); }
  int             sizeofPayload() const { return tag.extent() - sizeof(odfXTC); }
  odfInXtc*       next()                { return (odfInXtc*) tag.next(); }
  const odfInXtc* next()          const { return (const odfInXtc*) tag.end(); }

  odfDamage damage;
  odfSrc    src;
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
typedef odfSrc                Src;
typedef odfTypeNumPrimary     TypeNumPrimary;
typedef odfTypeId             TypeId;
typedef odfTypeIdQualified    TypeIdQualified;
typedef odfTC                 TC;
typedef odfXTC                XTC;
typedef odfInXtc              InXtc;

}

#endif
