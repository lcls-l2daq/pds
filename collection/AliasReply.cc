#include "pds/collection/AliasReply.hh"
#include <stdio.h>

using namespace Pds;

AliasReply::AliasReply() :
  Message(Message::Alias, sizeof(AliasReply)-MAX_ALIASES*sizeof(SrcAlias))
{
}

AliasReply::AliasReply(const std::list<SrcAlias>& aliases) :
  Message(Message::Alias, sizeof(AliasReply)+(aliases.size()-MAX_ALIASES)*sizeof(SrcAlias))
{
  SrcAlias* p = _aliases;
  SrcAlias* end = p+MAX_ALIASES;
  for(std::list<SrcAlias>::const_iterator it = aliases.begin();
      it != aliases.end();
      it++)
    if (p < end) {
      *p++ = *it;
    } else {
      printf("Error: exceeded MAX_ALIASES (%d) in %s\n", MAX_ALIASES, __FUNCTION__);
      break;
    }
}

unsigned AliasReply::naliases() const { return payload()/sizeof(SrcAlias); }

const SrcAlias& AliasReply::alias(unsigned i) const
{
  return _aliases[i];
}
