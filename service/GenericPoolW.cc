#include "GenericPoolW.hh"

#include <stdio.h>

using namespace Pds;

GenericPoolW::GenericPoolW(size_t sizeofObject, int numberofObjects) :
  GenericPool(sizeofObject, numberofObjects),
  _sem(Semaphore::EMPTY)
{
  //  Pool::populate() is done by GenericPool::ctor
  int depth=0;
  do { _sem.give(); } while(++depth < numberofObjects);
}

GenericPoolW::~GenericPoolW()
{
}

void* GenericPoolW::deque()
{
  _sem.take();
  void* p = GenericPool::deque();
  while( p == NULL ) {  // this should never happen
    printf("GenericPoolW::deque returned NULL with depth %d.  Depleting semaphore.\n", depth());
    _sem.take();
    p = GenericPool::deque();
  }
  return p;
}

void GenericPoolW::enque(PoolEntry* entry)
{
  GenericPool::enque(entry);
  _sem.give();
}
