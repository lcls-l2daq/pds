#include "Query.hh"

using namespace Pds;

Query::Query( Type type, unsigned size ) :
  Message( Message::Query, size ),
  _type(type)
{}

Query::Type Query::type() const
{ return _type; }


PartitionGroup::PartitionGroup(unsigned partitionid) :
  Query( Query::Group, sizeof(PartitionGroup) ),
  _partitionid(partitionid)
{}

unsigned PartitionGroup::partitionid() const
{ return _partitionid; }


PartitionAllocation::PartitionAllocation() :
  Query( Query::Partition, sizeof(PartitionAllocation) )
{}

PartitionAllocation::PartitionAllocation(const Allocation& alloc) :
  Query( Query::Partition, sizeof(PartitionAllocation)+alloc.size()-sizeof(Allocation) ),
  _allocation(alloc)
{}

const Allocation& PartitionAllocation::allocation() const
{ return _allocation; }

