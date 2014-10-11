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

AliasConfigType* AliasFactory::config(const PartitionConfigType* partn) const
{
  std::vector<Alias::SrcAlias> sources;
  for(std::list<Alias::SrcAlias>::const_iterator it=_list.begin();
      it!=_list.end(); it++) {
    bool lskip=(partn!=0);
    if (partn) {
      ndarray<const Partition::Source,1> s(partn->sources());
      for(unsigned i=0; i<s.size(); i++)
	if (it->src()==s[i].src()) {
	  lskip=false; 
	  break;
	}
    }
    if (!lskip)
      sources.push_back(*it);
  }

  unsigned sz = sizeof(AliasConfigType)+sources.size()*sizeof(Alias::SrcAlias);
  char* p = new char[sz];
  return new(p) AliasConfigType(sources.size(),sources.data());
}
