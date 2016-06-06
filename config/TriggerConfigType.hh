#ifndef Pds_TriggerConfigType_hh
#define Pds_TriggerConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/trigger.ddl.h"

typedef Pds::TriggerData::ConfigV1 TriggerConfigType;
typedef Pds::TriggerData::L0SelectV1 L0SelectType;

static Pds::TypeId _trgConfigType(Pds::TypeId::Id_TriggerConfig,
                                  TriggerConfigType::Version);

#endif
