#include "Occurrence.hh"

#include "pds/service/Pool.hh"

using namespace Pds;

Occurrence::Occurrence(OccurrenceId::Value id,
		       unsigned            size) :
  Message  (Message::Occurrence, size),
  _id      (id)
{}

OccurrenceId::Value Occurrence::id      () const { return _id; }

void* Occurrence::operator new(size_t size, Pool* pool)
{ return pool->alloc(size); }

void Occurrence::operator delete(void* buffer)
{
  PoolEntry* entry = PoolEntry::entry(buffer);
  if (entry->_pool) {
    Pool::free(buffer);
  } else {
    delete [] reinterpret_cast<char*>(entry);
  }
}
