#ifndef Pds_MonCds_hh
#define Pds_MonCds_hh

#include <sys/uio.h>

#include "pds/mon/MonDesc.hh"
#include "pds/mon/MonPort.hh"
#include "pds/service/Semaphore.hh"

namespace Pds {

  class MonEntry;
  class MonGroup;

  class MonCds {
  public:
    MonCds(const char* name);
    ~MonCds();
  
    void add   (MonGroup* group);
    void remove(MonGroup* group);

    //    MonPort::Type type() const;
    const MonDesc& desc() const;
    MonGroup* group(unsigned short g) const;
    MonGroup* group(const char* name);
    const MonEntry* entry(int signature) const;
    MonEntry* entry(int signature);
    unsigned short ngroups() const;
    unsigned short totalentries() const;
    unsigned short totaldescs() const;

    void reset();
    void showentries() const;

    //  serialize the description
    unsigned description(iovec*) const;

    Semaphore& payload_sem() const;
  private:
    void adjust();

  private:
    MonDesc _desc;
    unsigned short _maxgroups;
    unsigned short _unused;
    MonGroup** _groups;
    mutable Semaphore _payload_sem;
  };
};

#endif
