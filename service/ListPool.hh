#ifndef PDS_LISTPOOL
#define PDS_LISTPOOL

#include <new>
#include <stdio.h>
#include <stdlib.h>

#include "ListEntry.hh"

typedef unsigned size_t; // so that its not necessary to bring in <stddef.h>...

namespace Pds {
class ListPool
{
        public:
                ListPool(size_t);
                ~ListPool();
                void* alloc(size_t);
                void  free(void*);
                void shrink(void *, size_t);
                int  removes() const;
                int  inserts() const;
                int  empties() const;
                int  getsize() const;
                void dump();
        private:
                void          _isEmpty();
                ListEntry*          _listentries;    // Listhead of allocated buffers
                int                    _removes;        // # of queue removes
                int                    _empties;        // # of queue inserts that failed
                int                    _inserts;        // # of queue inserts
                char *                 _pool;               // memory to be allocated 
                size_t                 _size;
  
  };
}


inline int Pds::ListPool::getsize() const
{
	return _size;
}

inline int Pds::ListPool::removes() const
{
        return _removes;
}


inline int Pds::ListPool::empties() const
{
        return _empties;
}


inline void Pds::ListPool::_isEmpty()
  {
  _empties++;
  }

inline int Pds::ListPool::inserts() const
{
        return _inserts;
}

#endif
