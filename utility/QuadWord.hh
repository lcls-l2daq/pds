#ifndef Pds_QuadWord_included
#define Pds_QuadWord_included

#include "pds/service/ODMGTypes.hh"

namespace Pds {
class QuadWord {

public:

  QuadWord( d_ULong upper, d_ULong lower );
  QuadWord( );
  QuadWord( const QuadWord& );
  ~QuadWord( );

  const d_ULong& upperword() const { return _upperword; }
  const d_ULong& lowerword() const { return _lowerword; }

  QuadWord       operator*(  d_ULong  multiplier );
  QuadWord       operator/(  d_UShort divisor );
  d_UShort          operator%(  d_UShort divisor );
  QuadWord       operator<<( d_ULong  shift );
  QuadWord       operator+(  QuadWord addend );
  QuadWord       operator-(  QuadWord addend );

private:

  QuadWord Divide( d_UShort divisor, d_UShort &remainder);

  inline d_ULong Extract( d_ULong source, d_UShort start, d_UShort end);
  inline d_ULong Insert(  d_ULong source, d_ULong arg, d_UShort start, d_UShort end);

  d_ULong _upperword;
  d_ULong _lowerword;

};
}

inline d_ULong Pds::QuadWord::Extract(d_ULong source, d_UShort start, d_UShort end)
{
  if (start > end) return 0xffffffff;
  source = source << (31 - end);
  source = source >> (31 - end + start);
  return source;
}

inline d_ULong Pds::QuadWord::Insert(d_ULong source, d_ULong arg, d_UShort start, d_UShort end)
{
  d_ULong dummy1,dummy2;
  d_ULong mask = 0xffffffff;
  if (start > end) return 0xffffffff;
  dummy1 = mask << start;
  dummy2 = mask >> (31 - end);
  mask   = dummy1 & dummy2;
  arg    = arg << start;
  arg    = arg & mask;
  source = source & ~mask;
  source = source |  arg;
  return source;
}

#endif  // QuadWord_included
