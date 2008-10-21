#include "GenericPoolW.hh"

using namespace Pds;

GenericPoolW::GenericPoolW(size_t sizeofObject, int numberofObjects) :
  GenericPool(sizeofObject, numberofObjects),
  _sem(Semaphore::EMPTY),
  _depth(numberofObjects)
{
}

GenericPoolW::~GenericPoolW()
{
}

void* GenericPoolW::deque()
{
  if (!_depth--)
    _sem.take();
  return GenericPool::deque();
}

void GenericPoolW::enque(PoolEntry* entry)
{
  GenericPool::enque(entry);
  if (!++_depth)
    _sem.give();
}
