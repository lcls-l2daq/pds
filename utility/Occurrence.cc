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
{
  strncpy(_msg, msg, MaxMsgLength); 
  _msg[MaxMsgLength-1] = 0;
}

int  UserMessage::remaining() const
{
  return MaxMsgLength-1 - strlen(_msg);
}

void UserMessage::append(const char* msg)
{ 
  int l = strlen(_msg);
  strncpy(_msg+l, msg, MaxMsgLength-l);
  _msg[MaxMsgLength-1] = 0;
}

DataFileOpened::DataFileOpened(unsigned _expt,
                               unsigned _run,
                               unsigned _stream,
                               unsigned _chunk,
                               char* _host,
                               char* _path) :
  Occurrence(OccurrenceId::DataFileOpened,
  sizeof(DataFileOpened)),
  expt  (_expt  ),
  run   (_run   ),
  stream(_stream),
  chunk (_chunk )
{
  strncpy(host, _host, sizeof(host)-1);
  strncpy(path, _path, sizeof(path)-1);
}

EvrCommandRequest::EvrCommandRequest(const std::vector<unsigned>& codes) :
  Occurrence(OccurrenceId::EvrCommandRequest,sizeof(EvrCommand)+codes.size()*sizeof(unsigned)),
  forward   (1),
  ncodes    (codes.size())
{
  unsigned* p = reinterpret_cast<unsigned*>(this+1);
  for(unsigned i=0; i<ncodes; i++)
    p[i] = codes[i];
}

std::vector<unsigned> EvrCommandRequest::eventCodes() const
{
  const unsigned* p = reinterpret_cast<const unsigned*>(this+1);
  std::vector<unsigned> codes(ncodes);
  for(unsigned i=0; i<ncodes; i++)
    codes[i] = p[i];
  return codes;
}

RegisterPayload::RegisterPayload(unsigned v) :
  Occurrence(OccurrenceId::RegisterPayload,sizeof(RegisterPayload)),
  payload   (v)
{
}
