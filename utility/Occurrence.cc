#include "Occurrence.hh"

#include "pds/service/Pool.hh"

#include <stdio.h>
#include <string.h>

using namespace Pds;

Occurrence::Occurrence(OccurrenceId::Value id,
		       unsigned            size) :
  Message  (Message::Occurrence, size),
  _id      (id)
{
}

Occurrence::Occurrence(const Occurrence& c) :
  Message  (Message::Occurrence, c.size()),
  _id      (c._id)
{
  memcpy(this+1, &c+1, c.size()-sizeof(Occurrence));
}

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

UserMessage::UserMessage() :
  Occurrence(OccurrenceId::UserMessage,
	     sizeof(UserMessage))
{ _msg[0]=0; }

UserMessage::UserMessage(const char* msg) :
  Occurrence(OccurrenceId::UserMessage,
	     sizeof(UserMessage))
{ strncpy(_msg, msg, MaxMsgLength); }

void UserMessage::append(const char* msg)
{ 
  char* m = _msg + strlen(_msg);
  strncpy(m, msg, _msg+MaxMsgLength-m-1);
}
