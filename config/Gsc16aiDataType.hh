#ifndef GSC16AIDATATYPE_HH
#define GSC16AIDATATYPE_HH

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/gsc16ai.ddl.h"

typedef Pds::Gsc16ai::DataV1 Gsc16aiDataType;

static Pds::TypeId _gsc16aiDataType(Pds::TypeId::Id_Gsc16aiData,
                                    Gsc16aiDataType::Version);

#endif
