#ifndef PDS_RINGENTRY_HH
#define PDS_RINGENTRY_HH

#include "Queue.hh"

#include <stddef.h>                     // for size_t

namespace Pds {

class RingPool;

class RingEntry : public Entry
{
public:
  RingEntry(RingPool*);
  ~RingEntry();

  void* operator new(size_t, void*);

private:
  friend class RingPool;
  friend class RingPoolW;
private:
  RingPool*  _pool;                  // point to the source ring pool
  unsigned      _tag;                   // Pool type identifier
};
}

/*
** ++
**
**
** --
*/

inline Pds::RingEntry::RingEntry(Pds::RingPool* pool) :
  Pds::Entry(),
  _pool(pool),
  _tag(2)
{
}

/*
** ++
**
**
** --
*/

inline Pds::RingEntry::~RingEntry()
{
}


/*
** ++
**
**
** --
*/

inline void* Pds::RingEntry::operator new(size_t, void* location)
{
  return location;
}

#endif
