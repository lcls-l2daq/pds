#include "GenericPoolW.hh"

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
  return GenericPool::deque();
}

void GenericPoolW::enque(PoolEntry* entry)
{
  GenericPool::enque(entry);
  _sem.give();
}
