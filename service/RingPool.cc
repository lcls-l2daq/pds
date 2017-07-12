/*
** ++
**  Package:
**	Service
**
**  Abstract:
**      A ring buffer based memory allocator class
**
**  Author: Satyam Vaghani
**  Date: December 2000
**  Dependencies: RingPool.hh, RingEntry.hh
**
**  Description: A fast memory allocator. Space efficiency is given secondary
**  importance in this implementation. We take advantage of the fact that
**  a ring buffer eliminates the need to 'look back' while allocating new memory
**  Fragmented memory is not considered for new allocations until contiguous
**  fragments get together (at which time these are added to the free pool
**  in a constant time O(1) calculation. Result: None of the operations in this
**  class are worse than O(1).
**
**  RingPool consists of a contiguous area of memory where allocations
**  are done. The class maintains allocation information inside this memory
**  area itself. Allocation information is maintained in RingEntry structures
**  located at the head of each allocation. Hence there is a
**  sizeof(RingEntry) overhead with each allocation.
**
**  Note that any allocations which are not freed will block further allocation
**  when the head returns to that point in the ring.  Thus, the ringpool requires
**  timely return of all allocations to avoid a deadlock.
**
**
**  Revision History:
**      000 - January 23, 2001 - Ric Claus
**            Significantly modified this class to improve performance and
**            robustness in a multithreaded environment.
**      001 - July    30, 2001 - Ric Claus
**            More work related to making the class thread safe.
*/

#include <stdio.h>

#include "RingPool.hh"
#include "RingEntry.hh"
#include "Exc.hh"

using namespace Pds;


RingPool::RingPool(const size_t size,
                         const size_t wrap) :
  _allocatedList(),
  _pool(new char[size]),                // Naturally quadword aligned
  _size(size),
  _wrap(&_pool[size - wrap - sizeof(RingEntry)]),
  _next(_pool),
  _freeFn(&RingPool::_free),
  _frees(0),
  _allocs(0),
  _empties(0),
  _atHeads(0),
  _maxSize(0)
{
  if (!_pool)                 _bugCheck(size);
  if (!wrap || wrap >= size)  _bugCheck(wrap, size);
  if (size & 0x7)             _bugCheck("size", size);
  if (wrap & 0x7)             _bugCheck("wrap", wrap);
}

RingPool::RingPool(const size_t size,
                         const size_t wrap,
                         VoidFuncPtr  freeFn) :
  _allocatedList(),
  _pool(new char[size]),                // Naturally quadword aligned
  _size(size),
  _wrap(&_pool[size - wrap - sizeof(RingEntry)]),
  _next(_pool),
  _freeFn(freeFn),
  _frees(0),
  _allocs(0),
  _empties(0),
  _atHeads(0),
  _maxSize(0)
{
  if (!_pool)                 _bugCheck(size);
  if (!wrap || wrap >= size)  _bugCheck(wrap, size);
  if (size & 0x7)             _bugCheck("size", size);
  if (wrap & 0x7)             _bugCheck("wrap", wrap);
}

void RingPool::_bugCheck(const size_t size) const
{
  char msg[128];
  sprintf(msg, "RingPool::ctor: "
               "Insufficient heap available to create pool of size %u\n", (unsigned) size);
  BugCheck(msg);
}

void RingPool::_bugCheck(const size_t wrap, const size_t size) const
{
  char msg[128];
  sprintf(msg, "RingPool::ctor: "
               "Bad wrap value (%u): must be > 0 && < pool size %u\n",
          (unsigned) wrap, (unsigned) size);
  BugCheck(msg);
}

void RingPool::_bugCheck(const char* name, const size_t size) const
{
  char msg[128];
  sprintf(msg, "RingPool::ctor: "
               "Bad %s value (%u) is not divisible by eight\n",
          name, (unsigned) size);
  BugCheck(msg);
}

/*
**
**  This method returns unused space to the pool. However, the amount of
**  free space is to be decided by the user. The user calls this method with the
**  actual amount of space that he/she used. In case the allocation which is to
**  be shrunk is next to the head pointer, the allocated payload size is reduced
**  to the requested size. In case the allocation is not next to the current
**  position of head, the class does not shrink it because the freed area is not
**  going to be used by the allocator anyway.
**
** 'buffer' must have been allocated by this class's alloc function above.
** 'size'   is rounded up to an integer number of quadwords to prevent
**          allocations from becoming non-quadword aligned
*/

void* RingPool::shrink(void*        buffer,
                          const size_t size)
{
  // Conditional commented out for performance reasons
  //if ((char*)&(_atTail())[1] == (char*)buffer)
  {
    size_t    sz = (size + 7) & ~0x7;   // Maintain quadword alignment
    _next = (char*)buffer+sz;
    if (sz > _maxSize)  _maxSize = sz;
  }
  return buffer;
}

/*
**
**  Allocation is very simple: check if there is enough space between the head
**  and tail pointers on the ring buffer. there should be at least
**  sizeof(RingEntry) plus the request amount of bytes free.
**
**  The function can be used in three ways:
**  - The first method is to allocate a specific amount of memory.
**      Supply some non-zero minSize and a zero retSize pointer.
**  - The second method is to allocate all the available memory.
**      Supply zero minSize and a non-zero retSize pointer.
**  - The third method is to allocate at least some minimum amount of memory,
**    and more if it is available.
**      Supply both arguments non-zero.
**
**  Note that the allocation method is not reentrant.  Multiple tasks/threads
**  attempting to allocate from the same ringPool will result in the same
**  allocation occasionally being handed to the multiple callers.  However,
**  allocating and freeing may be done in separate tasks/threads.  Shrinking
**  can only be done in the task/thread that is doing the allocating.
**
**  Note that this function is inlined.  By inlining, the extra work necessary
**  to handle the two cases should be optimized away by the compiler, resulting
**  in different functions appropriate to the different cases.
**
*/

inline void* RingPool::_alloc(const size_t minSize,
                                 size_t*      retSize)
{
  // Determine the head (a pointer into the pool where a new allocation may be
  // placed) and the tail (a pointer into the pool where the oldest allocation
  // resides).
  char* head = _next;
  char* tail = (char*)_atHead();

  // If the tail indicates the queue is empty, the pool is empty.  The head
  // will either be nonsense (in the case _atTail returned empty) or point right
  // after an allocation that was deleted just before _atHead was sampled.
  // In either case, it is fair to start at the top of the pool, for lack
  // of a better (e.g. "current") spot.
  if (tail == (char*)_empty())
  {
    head = _pool;
    tail = &head[_size];
  }
  else if (head > tail)
  {
    if (head > _wrap)
    {
      head = _pool;
    }
    else
    {
      tail = &_pool[_size];
    }
  }

  // Make sure that at least an RingEntry and the requested size will fit
  ssize_t size = tail - (head + sizeof(RingEntry));
  if (size > (ssize_t)minSize)
  {
    // Allocate a buffer and prepend it with a header
    RingEntry* entry = new(head) RingEntry(this);

    // Put the entry on the tail of the queue
    _insert(entry);

    // Save the size of the returned space in order to place the next allocation
    char* result = (char*)&entry[1];
    _next = result + ( retSize ? (*retSize = size) : minSize );

    // Return the allocated buffer
    return result;
  }
  else
  {
    ++_allocs;                          // Bumped, to be like Heap, etc
    ++_empties;

    if (size && retSize)  *retSize = 0;

    return 0;
  }
}

/*
**
**  This is here because inlining the one above with this name doesn't cause it
**  to appear as a global symbol in the object file.  We want it both inlined
**  in the methods below and be public.
**
**  This function attempts to allocate at least the number of bytes given by the
**  minSize argument.  If it succeeds, it returns a pointer to the allocated
**  buffer and the number of bytes it actually allocated in the location
**  pointed to by retSize.
**  If it doesn't, it returns the NULL pointer.
**
*/

void* RingPool::alloc(const size_t minSize,
                         size_t*      retSize)
{
  return _alloc(minSize, retSize);
}

/*
**  This function attempts to allocate the number of bytes given by the size
**  argument.  If it succeeds, it returns a pointer to the allocated buffer.
**  If it doesn't, it returns the NULL pointer.
*/

void* RingPool::alloc(const size_t size)
{
  // Maybe some extra to maintain quadword alignment
  size_t sz = (size + 7) & ~0x7;
  if (sz > _maxSize)  _maxSize = sz;
  return _alloc(sz, 0);
}

/*
**  This function attempts to allocate all the bytes available in the pool.
**  The minimum number of bytes available must be large enough to fit an
**  RingEntry.  If at least that many are available, the function returns a
**  pointer to the allocated buffer and the number of bytes it was able to
**  allocate in the size argument.  The returned size does NOT include the size
**  of an RingEntry.  If the function is not able to allocate more than
**  sizeof(RingEntry) bytes, it returns the NULL pointer and zero in the size
**  argument.
*/

void* RingPool::alloc(size_t* size)
{
  return _alloc(0, size);
}

/*
**
**  This method frees an existing allocation.  It also removes the corresponding
**  RingEntry structure from the linked list of RingEntry structures.
**
**  In the special case that the entry being freed is the one at the head of the
**  linked list, the tail pointer can be moved up to the next entry in the
**  linked list.  In the case that it isn't, it is just removed from the linked
**  list.  This has the effect of giving the ownership of the space occupied by
**  the entry being freed to the previous entry in the linked list.  When it is
**  freed (and it is at the head of the list), the space from what was once two
**  entries will be returned to the pool.
**
**  In the special case that the entry being freed is the only one in the linked
**  list, an extra step must be taken to ensure that the tail doesn't end up
**  pointing at the listhead.  The proper value for it to receive is the value
**  of the head.  Care must be taken not to pick up a value of the head after
**  the decision that the list contains only one entry is made or one risks
**  inadvertantly freeing an entry that was allocated during a task switch
**  between the compare and the fetch of the head.  Therefore we fetch the head
**  before the compare, even when we don't use it.  I think this is guarenteed
**  to work, even in light of compiler optimizations and cache/execution order
**  features of the PPC.
*/

inline void RingPool::_free(RingEntry* entry,
                               void*         buffer)
{
  // If the entry being freed is the same as the entry at the head of the queue,
  // then we can advance the tail pointer up to the next entry
  if (entry == _atHead())  ++_atHeads;

  // Remove the entry from head of the queue
  _remove(entry);
}

/*
**
** This is a static function that is used to set up the this pointer and call
** the real free function in the proper context.  The compiler should inline
** the real free function, causing this technique not to add any extra overhead.
**
*/

void RingPool::_free(void* entry,
                        void* buffer)
{
  RingEntry* ringEntry = (RingEntry*)entry;

  RingPool*  ringPool  = (RingPool*)ringEntry->_pool;
  ringPool->_free(ringEntry, buffer);
}

/*
**
**  Static function to return a single buffer (as specified by the argument)
**  to the ring pool. Note that this memory MUST have been allocated via
**  a call to the "alloc" function described above. In this instance the
**  memory is returned by pointing to the opaque control structure allocated
**  just behind the buffer. This structure contains a pointer to the
**  ring pool from which the buffer was allocated. This  operation is
**  re-entrant, allowing this function to called at both task and ISR level.
**  The function returns NO value.
**
*/

#include "Pool.hh"

#ifdef VXWORKS
#include <logLib.h>
#endif

void RingPool::free(void* buffer)
{
  RingEntry* entry = (RingEntry*)buffer - 1;

  switch (entry->_tag)
  {
    case 0:
              break;      
    case 2:   (*((RingPool*)entry->_pool)->_freeFn)((void*)entry, buffer);
              break;
    default:
#ifdef VXWORKS
              if ((entry->_tag != 0xffffffff) && (entry->_tag & 0xffff))
              {
                logMsg("RingPool::free: pool: tag = %08x, buffer = %x\n",
                       entry->_tag, (int)buffer,0,0,0,0);
              }
#endif
              Pool::free(buffer);
              break;
  }
}

/*
**
**  Destructor.
**
*/

RingPool::~RingPool()
{
  delete[] _pool;
}

/*
**
**  The dump method prints the RingPool usage statistics
**
*/

void RingPool::dump(int level)
{
  printf("\n"
         "Ring Pool (location = %p, size = 0x%08x) usage:\n"
         "---------------------------------------------------------\n"
"   Bytes     Bytes                            Ordered\n"
" Allocated   Free    Allocates     Frees       Frees      Empty   In Use\n"
" --------- -------- ----------- ----------- ----------- --------- ------\n",
         _pool, (unsigned) _size);

  RingEntry* atTail = _atTail();
  RingEntry* atHead = _atHead();
  char*         head   = _next;
  char*         tail   = (char*)atHead;
  if (tail == (char*)_empty())
  {
    head = _pool;
    tail = &head[_size];
  }
  size_t size = head < tail ? tail - head : _size - (head - tail);
  if ((head == tail) && (tail != (char*)_empty()))  size = 0;
  printf("  %08x %08x %11u %11u %11u %8u %6d\n",
         (unsigned) ( _size - size ),
         (unsigned) size,
         _allocs,
         _frees,
         _atHeads,
         _empties,
         _allocs - _frees - _empties);

  if (level)
  {
    size_t wrapSize = &_pool[_size] - _wrap;
    printf("\n"
           " empty  %p, top  %p, wrap   %p, bottom %p\n"
           " atTail %p, head %p, atHead %p, tail   %p\n"
           " max allocation size seen: %8u = 0x%08x\n"
           " wrap size:                %8u = 0x%08x\n",
           _empty(), _pool, _wrap, &_pool[_size],
           atTail, head, atHead, tail,
           (unsigned) _maxSize, (unsigned) _maxSize, (unsigned) wrapSize, (unsigned) wrapSize);

    printf("\n Allocations:\n");
    printf(" Entry   Location     size\n"
           " -----  ----------  --------\n");
    unsigned      cnt   = 0;
    RingEntry* entry = atHead;
    while (entry != _empty())
    {
      char*  buffer = (char*)&entry[1];
      entry         = (RingEntry*)entry->next();
      char*  end    = entry != _empty() ? (char*)entry : head;
      size_t size   = (end > buffer ? end : &_pool[_size]) - buffer;

      printf(" %5u   %p  %8u\n", cnt++, buffer, (unsigned) size);
    }
    if (cnt == 0)  printf(" none\n");
  }
}
