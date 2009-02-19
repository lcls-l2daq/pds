#ifndef Pds_MonENTRYFACTORY_HH
#define Pds_MonENTRYFACTORY_HH

namespace Pds {

  class MonEntry;
  class MonDescEntry;

  class MonEntryFactory {
  public:
    static MonEntry* entry(const MonDescEntry& desc);
  };
};

#endif
