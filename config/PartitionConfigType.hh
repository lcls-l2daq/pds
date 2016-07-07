#ifndef Pds_PartitionConfigType_hh
#define Pds_PartitionConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/partition.ddl.h"

typedef Pds::Partition::ConfigV2 PartitionConfigType;

static Pds::TypeId _partitionConfigType(Pds::TypeId::Id_PartitionConfig,
                                        PartitionConfigType::Version);

#endif
