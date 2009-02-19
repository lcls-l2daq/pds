#include <sys/uio.h>

#include "pds/mon/MonMessage.hh"

using namespace Pds;

MonMessage::MonMessage(Type type, unsigned payload) : 
  _type(type),
  _payload(payload)
{}

MonMessage::Type MonMessage::type() const {return _type;}

void MonMessage::type(Type t) {_type = t;}

unsigned MonMessage::payload() const {return _payload;}

void MonMessage::payload(const iovec* iov, unsigned iovcnt) 
{
  unsigned size = 0;
  for (unsigned n=0; n<iovcnt; n++, iov++) {
    size += (*iov).iov_len;
  }
  _payload = size;
}

void MonMessage::payload(unsigned size) 
{
  _payload = size;
}
