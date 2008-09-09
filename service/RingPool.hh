/*
** ++
**  Package:
**	Service
**
**  Abstract:
**     Definition of RingPool.
**
**  Author:
**      Ric Claus, SLAC
**
**  Creation Date:
**	000 - January 23, 2001
**
**  Revision History:
**	None.
**
** --
*/

#ifndef PDS_RINGPOOL_HH
#define PDS_RINGPOOL_HH

#include <stddef.h>                     // for size_t
#include "Queue.hh"

typedef void (*VoidFuncPtr)(void* entry, void* buffer);

namespace Pds {

class RingEntry;

class RingPool
{
public:
  RingPool(const size_t size, const size_t wrap);
  RingPool(const size_t size, const size_t wrap, VoidFuncPtr freeFn);
  ~RingPool();
public:
         void*  alloc(const size_t minSize, size_t* size);
         void*  alloc(const size_t minSize);
         void*  alloc(size_t* size);
  static void   free(void* buffer);
         void*  shrink(void* buffer, const size_t);
public:
  size_t        size()    const;
  unsigned      allocs()  const;
  unsigned      frees()   const;
  unsigned      empties() const;
  void          dump(int level);
private:
  friend class RingEntry;
  friend class RingPoolW;
private:
  void*         _alloc(const size_t minSize, size_t* size);
  static void   _free(void* entry, void* buffer);
  void          _free(RingEntry*, void* buffer);
  void          _insert(RingEntry*);
  RingEntry* _remove(RingEntry*);
  RingEntry* _atHead() const;
  RingEntry* _atTail() const;
  RingEntry* _empty()  const;
  void          _isEmpty();
  void          _bugCheck(const size_t size)                    const;
  void          _bugCheck(const size_t wrap, const size_t size) const;
  void          _bugCheck(const char*  name, const size_t size) const;
private:
  Queue<RingEntry> _allocatedList;// Listhead of free buffer entries
  char* const            _pool;         // Pool from which to allocate
  const size_t           _size;         // Size of the pool
  char* const            _wrap;         // Point after which to go back to top
  char*                  _next;         // Place for the next allocation
  const VoidFuncPtr      _freeFn;       // Function to call to free allocations
  unsigned               _frees;        // # of frees
  unsigned               _allocs;       // # of allocates
  unsigned               _empties;      // # of times pool was empty
  unsigned               _atHeads;      // # of time object removed at the head
  size_t                 _maxSize;      // Size of largest allocation seen
};
}

/*
** ++
**
**    Put an allocated entry onto the allocated queue.
**
** --
*/

inline void Pds::RingPool::_insert(Pds::RingEntry* entry)
{
  ++_allocs;
  _allocatedList.insert(entry);
}

/*
** ++
**
**    Remove an allocated entry from the allocated queue.  This is not
**    necesarily the entry at the head or the tail of the queue.
**
** --
*/

inline Pds::RingEntry* Pds::RingPool::_remove(Pds::RingEntry* entry)
{
  ++_frees;
  return _allocatedList.remove(entry);
}

/*
** ++
**
**   Returns a pointer to the Pds::RingEntry at the head of the list.
**
** --
*/

inline Pds::RingEntry* Pds::RingPool::_atHead() const
{
  return (Pds::RingEntry*)_allocatedList.atHead();
}

/*
** ++
**
**   Returns a pointer to the Pds::RingEntry at the tail of the list.
**
** --
*/

inline Pds::RingEntry* Pds::RingPool::_atTail() const
{
  return (Pds::RingEntry*)_allocatedList.atTail();
}

/*
** ++
**
**   Returns a pointer which can be compared to the entries returned by
**   the "_remove()" function.  If the pointers are equal the pool is empty.
**
** --
*/

inline Pds::RingEntry* Pds::RingPool::_empty() const
{
  return (Pds::RingEntry*)_allocatedList.empty();
}

/*
** ++
**
**    Return the size of the pool.
**
** --
*/

inline size_t Pds::RingPool::size() const
{
  return _size;
}

/*
** ++
**
**    Return the number times the "_insert" function was called.
**
** --
*/

inline unsigned Pds::RingPool::allocs() const
{
  return _allocs;
}

/*
** ++
**
**    Return the number times the "_remove" function was called.
**
** --
*/

inline unsigned Pds::RingPool::frees() const
{
  return _frees;
}

/*
** ++
**
**    Return the number times the "_isEmpty" function was called.
**
** --
*/

inline unsigned Pds::RingPool::empties() const
{
  return _empties;
}

#endif
