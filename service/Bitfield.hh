// updated 3/6/98 Ian Adam - change methods to statics.
// Note that source is passed by value in Insert => to set bits in x use, for example
// x = Bitfield::Insert(x,0xC,0,3)  - sets byte 0 of (unsigned)x to 1100=0xC  

#ifndef PDS_BITFIELD_HH
#define PDS_BITFIELD_HH

namespace Pds {
class Bitfield {
public:
  inline static unsigned Extract(unsigned source, unsigned short start, unsigned short end);
  inline static unsigned Insert(unsigned source, unsigned arg, unsigned short start, unsigned short end);
};
}

inline unsigned Pds::Bitfield::Extract(unsigned source, unsigned short start, unsigned short end)
{
  if (start > end) return 0xffffffff;
  source = source << (31 - end);
  source = source >> (31 - end + start);
  return source;
}

inline unsigned Pds::Bitfield::Insert(unsigned source, unsigned arg, unsigned short start, unsigned short end)
{
  unsigned dummy1,dummy2;
  unsigned mask = 0xffffffff;
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


#endif
