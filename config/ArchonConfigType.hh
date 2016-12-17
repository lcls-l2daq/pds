#ifndef ArchonConfigType_hh
#define ArchonConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/archon.ddl.h"

typedef Pds::Archon::ConfigV1 ArchonConfigType;

static Pds::TypeId _archonConfigType(Pds::TypeId::Id_ArchonConfig,
				     ArchonConfigType::Version);

#endif
