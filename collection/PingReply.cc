#include "pds/collection/PingReply.hh"

#include <stdio.h>

using namespace Pds;

static const uint32_t ReadyMask   =  (1<<31);
static const uint32_t PayloadMask = ~(1<<31);

PingReply::PingReply() :
  Message    (Message::Ping, sizeof(PingReply)-MAX_SOURCES*sizeof(Src)),
  _maxPayload(0)
{
}

PingReply::PingReply(const std::list<Src>& sources,
                     unsigned              maxPayload) :
  Message    (Message::Ping, sizeof(PingReply)+(sources.size()-MAX_SOURCES)*sizeof(Src)),
  _maxPayload(maxPayload&PayloadMask)
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

bool PingReply::ready() const { return _maxPayload&ReadyMask; }

void PingReply::ready(bool r) { 
  if (r)
    _maxPayload |= ReadyMask;
  else
    _maxPayload &= ~ReadyMask;
}

unsigned PingReply::maxPayload() const { return _maxPayload&PayloadMask; }
