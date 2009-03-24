#ifndef Pds_SourceLevel_hh
#define Pds_SourceLevel_hh

#include "pds/collection/CollectionSource.hh"

namespace Pds {

  class Controller;
  class Allocation;

  class SourceLevel: public CollectionSource {
  public:
    enum { MaxPartitions=16 };
    SourceLevel();
    virtual ~SourceLevel();
  public:
    bool connect(int);
  public:
    void dump() const;
  private:
    virtual void message(const Node& hdr, const Message& msg);
    void _assign_partition(const Node& hdr, const Ins& dst);
    void _resign_partition(const Node& hdr);
    void _verify_partition(const Node& hdr, const Allocation& alloc);
    void _show_partition(unsigned partition, const Ins& dst);
  private:
    Controller* _control;
  };

};

#endif
