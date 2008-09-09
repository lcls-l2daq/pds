/*

 * A doubly linked list based memory allocator class
 * 
 * Author: Satyam Vaghani
 * Date: December 2000
 * Dependencies: ListPool.hh, ListEntry.hh
 *
 * Description: This class evolved while trying to implement a BitPool
 * allocator. The ListPool allocator is similar to the RingPool allocator 
 * except that this class tries to allocate on a 'first-fit' basis. Thus 
 * allocation can possibly happen in memory fragments while the RingPool
 * allocator will not allocate iinside fragments.
 *
 * ListPool consists of a contiguous area of memory where allocations 
 * are done. The class maintains allocation information inside this memory
 * area itself. Allocation information is maintained in ListEntry structures
 * located at the head of each allocation. Hence there is a sizeof(ListEntry)
 * overhead with each allocation.
 *

*/


#include "ListPool.hh"

#define HEADERSIZE        sizeof(ListEntry)

using namespace Pds;

ListPool::ListPool(size_t poolsize)
{
	_removes = _inserts = _empties = 0;
	_size = poolsize;
	_listentries = (ListEntry *)0;
        _pool = (char *)malloc(poolsize); 
}

/*
 Allocation is the most complicated step in this implementation: the allocator checks for the first memory fragment which can fulfil the allocation request. This area can be located at the end of all allocations or in the middle of two active allocations. there should be at least HEADERSIZE (sizeof(RingEntry)) + request amount of bytes free. If space does exist, then a header is put in front of the allocation.
*/
void* ListPool::alloc(size_t chunksize)
{
   ListEntry *current = _listentries, *previous = (ListEntry *)0;
   unsigned i;

   while (current)
   {
	   if ( current->_flink && ( ((char *)(current->_flink) - (char*)current) - HEADERSIZE - current->_reserved) >= (chunksize+HEADERSIZE) )
	   {
			previous = current;	
			current = (ListEntry *)( (char *)current + HEADERSIZE + current->_reserved );
			current->_blink = previous;
			current->_flink = previous->_flink;
			previous->_flink = current;
			current->_reserved = chunksize;
			return ( (void *)( (char*)current +HEADERSIZE));
	   }
	   previous = current;
	   current = current->_flink;
			
   }
   if (previous)
   		i = ((char*)previous - _pool)+previous->_reserved+HEADERSIZE;
   else 
		i = 0;
   if ( i < _size)
		current = (ListEntry*)( _pool + i );
   else
		return (void *)0;

   if ( _size -  ((char*)current - _pool) >= chunksize+HEADERSIZE)
	{
		current->_blink = previous;
		current->_flink = (ListEntry *)0;
                if (previous) 
			previous->_flink = current;
		current->_reserved = chunksize;
		if ( !_listentries) _listentries = current;
		_inserts++;
		return ( (void *)((char*)current+HEADERSIZE) );
	}
	
	return (void*)0;
}

/* Unlike the RingPool class in which the shrink method shrinks only the boundary allocations, this shrink method shrinks any active allocation, thus creating memory fragments in which new allocations can (possibly) be accomodated
*/

void ListPool::shrink(void *buffer, size_t newsize)
{
	ListEntry *entry = (ListEntry*)((char*)buffer - HEADERSIZE);
	
	entry->_reserved = newsize;
}

/*

the free method removes the present allocation from the list of active allocations - thus implicitly making the area available for new allocations.

*/

void ListPool::free(void* buffer)
{
	ListEntry *entry = (ListEntry*)((char *) buffer - HEADERSIZE);

	if (_listentries == entry)
		_listentries = entry->_flink;
	if (entry->_blink)
		entry->_blink->_flink = entry->_flink;
	if (entry->_flink)
		entry->_flink->_blink = entry->_blink;
	_removes++;
	return;
}

/* the destructor is not really needed */

ListPool::~ListPool()
 {

	//std::free((void *)_pool);
 }

/* dump the usage statistics of the memory managed by ListPool */

void ListPool::dump()
  {
  printf("\nRing Pool usage...\n------------------\n"
"Removes  Allocates     Frees    Empty In Use\n"
"----------- ----------- ----- ------\n");

  printf("%11d %11d %5d %6d\n",
         removes(),
         inserts(),
         empties(),
         inserts() - removes() - empties());
  }
