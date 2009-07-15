#include "pds/collection/PingReply.hh"

using namespace Pds;

PingReply::PingReply(const std::list<Src>& sources) :
  Message(Message::Ping, sources.size()*sizeof(Src)+sizeof(Message))
{
  Src* p = _sources;
  for(std::list<Src>::const_iterator it = sources.begin();
      it != sources.end();
      it++)
    *p++ = *it;
}

unsigned PingReply::nsources() const { return payload()/sizeof(Src); }

const Src& PingReply::source(unsigned i) const
{
  return _sources[i];
}
