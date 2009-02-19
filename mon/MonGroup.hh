#ifndef Pds_MonGROUP_HH
#define Pds_MonGROUP_HH

#include "pds/mon/MonDesc.hh"

namespace Pds {

  class MonEntry;

  class MonGroup {
  public:
    MonGroup(const char* name);
    ~MonGroup();

    void add(MonEntry* entry);

    const MonDesc& desc() const;
    MonDesc& desc();

    const MonEntry* entry(unsigned short e) const;
    MonEntry* entry(unsigned short e);
    const MonEntry* entry(const char* name) const;
    MonEntry* entry(const char* name);

    unsigned short nentries() const;

  private:
    void adjust();

  private:
    MonDesc _desc;
    unsigned short _maxentries;
    unsigned short _unused;
    MonEntry** _entries;
  };
};

#endif
