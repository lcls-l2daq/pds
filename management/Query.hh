#ifndef Pds_Query_hh
#define Pds_Query_hh

#include "pds/collection/Message.hh"
#include "pds/utility/Transition.hh"

namespace Pds {

  class Query : public Message {
  public:
    enum Type { Group, Partition, Platform };
    Query( Type type, unsigned size );
  public:
    Type type() const;
  private:
    Type _type;
  };

  class PartitionGroup : public Query {
  public:
    PartitionGroup(unsigned partitionid);
  public:
    unsigned partitionid() const;
  private:
    unsigned _partitionid;
  };

  class PartitionAllocation : public Query {
  public:
    PartitionAllocation();
    PartitionAllocation(const Allocation&);
  public:
    const Allocation& allocation() const;
  private:
    Allocation _allocation;
  };
};

#endif
