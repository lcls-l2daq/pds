#include <sys/uio.h>

#include "pds/mon/MonMessage.hh"

using namespace Pds;

MonMessage::MonMessage(Type type, unsigned payload) : 
  _src (Level::Observer),
  _type(type),
  _payload(payload)
{}

MonMessage::MonMessage(const Src& src, Type type, unsigned payload) : 
  _src (src),
  _type(type),
  _payload(payload)
{}

Src MonMessage::src() const { return _src; }

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
