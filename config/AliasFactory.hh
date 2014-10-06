#ifndef Pds_AliasFactory_hh
#define Pds_AliasFactory_hh

#include "pds/config/AliasConfigType.hh"
#include <list>

namespace Pds {
  class AliasFactory {
  public:
    AliasFactory();    
    ~AliasFactory();
  public:
    void             insert(const Alias::SrcAlias&);
    const char*      lookup(const Src&) const;
    AliasConfigType* config() const;
  private:
    std::list<Alias::SrcAlias> _list;
  };
};

#endif
