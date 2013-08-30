#ifndef Pds_Gsc16aiConfigType_hh
#define Pds_Gsc16aiConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/gsc16ai.ddl.h"

typedef Pds::Gsc16ai::ConfigV1 Gsc16aiConfigType;

static Pds::TypeId _gsc16aiConfigType(Pds::TypeId::Id_Gsc16aiConfig,
                                      Gsc16aiConfigType::Version);

#endif
