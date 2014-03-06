#ifndef Pds_OceanOpticsConfigType_hh
#define Pds_OceanOpticsConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/oceanoptics.ddl.h"

typedef Pds::OceanOptics::ConfigV2 OceanOpticsConfigType;

static Pds::TypeId _oceanOpticsConfigType(Pds::TypeId::Id_OceanOpticsConfig,
          OceanOpticsConfigType::Version);

#endif
