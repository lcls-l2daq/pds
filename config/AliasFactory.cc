#include "pds/config/AliasFactory.hh"

using namespace Pds;

AliasFactory::AliasFactory() {}

AliasFactory::~AliasFactory() {}

void        AliasFactory::insert(const Alias::SrcAlias& alias)
{
  _list.push_back(alias);
  _list.sort();
  _list.unique();
}

const char* AliasFactory::lookup(const Src& src) const
{
  std::list<Alias::SrcAlias>::const_iterator it;
  for (it = _list.begin(); it != _list.end(); it++) {
    if (it->src() == src) {
      return (it->aliasName());
    }
  }
  // no match found
  return ((char *)NULL);
}

AliasConfigType* AliasFactory::config() const
{
  unsigned sz = sizeof(AliasConfigType)+_list.size()*sizeof(Alias::SrcAlias);
  char* p = new char[sz];
  AliasConfigType* r = reinterpret_cast<AliasConfigType*>(p);
  new (p) uint32_t(_list.size()); p += sizeof(uint32_t);
  for(std::list<Alias::SrcAlias>::const_iterator it=_list.begin(); it!=_list.end(); it++) {
    new (p) Alias::SrcAlias(*it);
    p += sizeof(Alias::SrcAlias);
  }
  return r;
}
