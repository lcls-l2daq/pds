/*
** ++
**  Package:
**	utility
**
**  Abstract:
**      The purpose of this class is to provide memory allocations without removing them
**      from the pool of available memory.  No cleanup is required, thus the memory is reused.
**      
**  Author:
**      Matt Weaver, SLAC, (650) 926-5147
**
**  Creation Date:
**	000 - June 2, 2002
**
**  Revision History:
**	None.
**
** --
*/

#ifndef Pds_ReusePool_hh
#define Pds_ReusePool_hh

#include "pds/service/GenericPool.hh"

namespace Pds {
class ReusePool : public GenericPool {
public:
  ReusePool(size_t sizeofObject, int numberofObjects);
  ReusePool(size_t sizeofObject, int numberofObjects, unsigned alignBoundary);
  ~ReusePool();

private:
  virtual void* deque();
  virtual void enque(PoolEntry*);
};

inline void* ReusePool::deque() {
  PoolEntry* entry = remove();
  insert(entry);
  return (void*)&entry[1];
}
}

inline void Pds::ReusePool::enque(Pds::PoolEntry* entry) {  // no op
}

#endif
