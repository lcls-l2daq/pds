#ifndef Pds_QuadWord_included
#define Pds_QuadWord_included

#include <stdint.h>

namespace Pds {
class QuadWord {

public:

  QuadWord( uint32_t upper, uint32_t lower );
  QuadWord( );
  QuadWord( const QuadWord& );
  ~QuadWord( );

  const uint32_t& upperword() const { return _upperword; }
  const uint32_t& lowerword() const { return _lowerword; }

  QuadWord       operator*(  uint32_t  multiplier );
  QuadWord       operator/(  uint16_t divisor );
  uint16_t       operator%(  uint16_t divisor );
  QuadWord       operator<<( uint32_t  shift );
  QuadWord       operator+(  QuadWord addend );
  QuadWord       operator-(  QuadWord addend );

private:

  QuadWord Divide( uint16_t divisor, uint16_t &remainder);

  inline uint32_t Extract( uint32_t source, uint16_t start, uint16_t end);
  inline uint32_t Insert(  uint32_t source, uint32_t arg, uint16_t start, uint16_t end);

  uint32_t _upperword;
  uint32_t _lowerword;

};
}

inline uint32_t Pds::QuadWord::Extract(uint32_t source, uint16_t start, uint16_t end)
{
  if (start > end) return 0xffffffff;
  source = source << (31 - end);
  source = source >> (31 - end + start);
  return source;
}

inline uint32_t Pds::QuadWord::Insert(uint32_t source, uint32_t arg, uint16_t start, uint16_t end)
{
  uint32_t dummy1,dummy2;
  uint32_t mask = 0xffffffff;
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
