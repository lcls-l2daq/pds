#ifndef Pds_TprDSConfigType_hh
#define Pds_TprDSConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/tprds.ddl.h"

typedef Pds::TprDSData::ConfigV1  TprDSConfigType;

static Pds::TypeId _tprdsConfigType(Pds::TypeId::Id_TprDSConfig,
                                    TprDSConfigType::Version);

#endif
