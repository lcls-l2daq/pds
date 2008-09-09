/*
** ++
**  Package:
**	Service
**
**  Abstract:
**
**  Author:
**      Michael Huffer, SLAC, (415) 926-4269
**
**  Creation Date:
**	000 - December 20 1,1997
**
**  Revision History:
**	None.
**
** --
*/

#ifndef PDS_POOL
#define PDS_POOL


#include "PoolEntry.hh"

namespace Pds {
class Pool
  {
  public:
    virtual ~Pool();
    Pool(size_t sizeofObject, int numberofOfObjects);
    Pool(size_t sizeofObject, int numberofOfObjects, unsigned alignBoundary);
    // Commented out by RiC 3/8/01 because gcc complains
    //void*         Pool::alloc(size_t size);
    //static void   Pool::free(void* buffer);
    void*         alloc(size_t size);
    static void   free(void* buffer);
    size_t        sizeofObject()    const;
    int           numberofObjects() const;
    int           numberofAllocs()  const;
    int           numberofFrees()   const;
  protected:
    size_t        sizeofAllocate()  const;
    virtual void* deque()                  = 0;
    virtual void  enque(PoolEntry*)     = 0;
    virtual void* allocate(size_t size)    = 0;
    void          populate();
  private:
    size_t        _sizeofObject;
    int           _numberofObjects;
    int           _numberofAllocs;
    int           _numberofFrees;
    int           _remaining;
    size_t        _quanta;
  };
}
/*
** ++
**
**
** --
*/

inline void* Pds::Pool::alloc(size_t size)
  {
  _numberofAllocs++;
  return (size > _sizeofObject) ? (void*)0 : deque();
  }

/*
** ++
**
**
** --
*/

inline void Pds::Pool::free(void* buffer)
  {
  Pds::Pool* pool = (Pds::PoolEntry::entry(buffer))->_pool;
  pool->enque(Pds::PoolEntry::entry(buffer));
  pool->_numberofFrees++;
  }

/*
** ++
**
**
** --
*/

inline Pds::Pool::~Pool()
  {
  }

/*
** ++
**
**
** --
*/

inline size_t Pds::Pool::sizeofObject() const
  {
  return _sizeofObject;
  }

/*
** ++
**
**
** --
*/

inline size_t Pds::Pool::sizeofAllocate() const
  {
  return _quanta;
  }

/*
** ++
**
**
** --
*/

inline int Pds::Pool::numberofObjects() const
  {
  return _numberofObjects;
  }

/*
** ++
**
**
** --
*/

inline int Pds::Pool::numberofAllocs() const
  {
  return _numberofAllocs;
  }

/*
** ++
**
**
** --
*/

inline int Pds::Pool::numberofFrees() const
  {
  return _numberofFrees;
  }

#endif
