/*
** ++
**  Package:
**	Service
**
**  Abstract:
**     Supplemental implementation functions for "RingPoolW.hh"
**
**  Author:
**      Ric Claus, SLAC
**
**  Creation Date:
**	000 - January 30, 2001
**
**  Revision History:
**	None.
**
** --
*/

#include "RingPoolW.hh"
#include "RingEntry.hh"

using namespace Pds;


/*
** ++
**
**    Constructor... It takes as a arguments the size of the pool to create
**    and the size of the wrap buffer required.
**
** --
*/

RingPoolW::RingPoolW(size_t size,
                           size_t wrap) :
  RingPool(size, wrap, &RingPoolW::_free),
  _stalls(0),
  _resumes(0),
  _stalled(0),
  _resource(Semaphore::EMPTY)
{
}

/*
** ++
**
**    Allocation function.  It takes as an argument the size of the space
**    to allocate.  This function can't fail.  If the allocation request
**    can't be met, it goes into resource wait mode until another thread
**    kicks it out by freeing a buffer.  It'll retry allocating the request
**    until it obtains it and then returns the buffer.  If the requested
**    size is larger than the pool less the size of an RingPoolEntry,
**    this function will stay in resource wait mode forever after all buffers
**    have been freed.  This is deemed better than testing whether the
**    request will fit in the pool on every call.
**
** --
*/

void* RingPoolW::alloc(size_t size)
{
  void*  buffer;
  _stalled = 1;  // Need to use the resource semaphore
  while (!(buffer = RingPool::alloc(size)))
  {
    ++_stalls;
    _resource.take();
    ++_resumes;
  }
  _stalled = 0; // Don't need to use the resource semaphore

  return buffer;
}

/*
** ++
**
**    Allocation function.  This function allocates all the free space in the
**    pool and returns this buffer to the caller.  It tells the caller the size
**    of the buffer through the 'size' argument.  This function is meant to be
**    used together with the shrink function below to return unneeded space back
**    to the pool.  If no space is available in the pool, this function goes
**    into resource wait mode until another thread kicks it out by freeing a
**    buffer.  It'll retry the allocation, probably only once, as it will have
**    obtained the space that was just freed.
**
** --
*/

void* RingPoolW::alloc(size_t* size)
{
  void*  buffer;
  _stalled = 1;  // Need to use the resource semaphore
  while (!(buffer = RingPool::alloc(size)))
  {
    ++_stalls;
    _resource.take();
    ++_resumes;
  }
  _stalled = 0; // Don't need to use the resource semaphore

  return buffer;
}

/*
** ++
**
**    Internal freeing function.  This function is called back by the public
**    free function in the base class.  It uses the base class's private free
**    function to return an allocation back to the pool and then releases
**    resource wait mode.
**
** --
*/

void RingPoolW::_free(void* entry, void* buffer)
{
  RingPoolW* pool = (RingPoolW*)((RingEntry*)entry)->_pool;

  RingPool::_free(entry, buffer);

  if (pool->_stalled)
  {
    pool->_resource.give();
  }
}

/*
** ++
**
**    Destructor.  Not too interesting.
**
** --
*/

RingPoolW::~RingPoolW()
{
}

/*
** ++
**
**    Dump function that is handy for debugging purposes.
**
** --
*/

void RingPoolW::dump(int level)
{
  RingPool::dump(level);

  printf(" RingPool resource wait stalls/resumes: %u/%u\n",
         _stalls, _resumes);

  printf(" _stalled = %u\n", _stalled);

#ifdef VXWORKS
  int idList=0;
  semInfo( *(SEM_ID*)&_resource, &idList, 1 );
  if (idList)
    printf("resource taken by 0x%x\n", idList);
  else
    printf("resource available\n");
#endif
}
