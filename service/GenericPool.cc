#include "GenericPool.hh"

using namespace Pds;

void GenericPool::dump() const 
{
  printf("  bounds %x  buffer %p  current %x\n",
	 _bounds, _buffer, _current);
  printf("  sizeofObject %x  numofObjects %x  allocs %x  frees %x\n",
	 sizeofObject(), numberofObjects(),
	 numberofAllocs(), numberofFrees() );
}
