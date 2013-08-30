#ifndef Pds_DiodeFexConfigType_hh
#define Pds_DiodeFexConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/lusi.ddl.h"

typedef Pds::Lusi::DiodeFexConfigV2 DiodeFexConfigType;

static Pds::TypeId _diodeFexConfigType(Pds::TypeId::Id_DiodeFexConfig,
				       DiodeFexConfigType::Version);

#endif
