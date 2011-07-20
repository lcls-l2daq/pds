#ifndef THist_hh
#define THist_hh

#include <time.h>

namespace Pds {
  class MonEntryTH1F;
  class THist {
  public:
    THist(const char* title);
    ~THist();
  public:
    void accumulate(const timespec& curr,
                    const timespec& prev);
    void print(unsigned) const;
  private:
    MonEntryTH1F* _entry;
  };
};

#endif
