#include "pds/collection/PingReply.hh"

using namespace Pds;

PingReply::PingReply() :
  Message(Message::Ping, sizeof(PingReply)-MAX_SOURCES*sizeof(Src)),
  _ready (false)
{
}

PingReply::PingReply(const std::list<Src>& sources) :
  Message(Message::Ping, sizeof(PingReply)+(sources.size()-MAX_SOURCES)*sizeof(Src)),
  _ready (false)
{
  Src* p = _sources;
  Src* end = p+MAX_SOURCES;
  for(std::list<Src>::const_iterator it = sources.begin();
      it != sources.end();
      it++)
    if (p < end)
      *p++ = *it;
    else
      printf("PingReply exceeded MAX_SOURCES\n");
}

unsigned PingReply::nsources() const { return payload()/sizeof(Src); }

const Src& PingReply::source(unsigned i) const
{
  return _sources[i];
}

bool PingReply::ready() const { return _ready; }

void PingReply::ready(bool r) { _ready = r; }
