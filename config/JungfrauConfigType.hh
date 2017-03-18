#ifndef JungfrauConfigType_hh
#define JungfrauConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/jungfrau.ddl.h"

typedef Pds::Jungfrau::ConfigV2 JungfrauConfigType;

static Pds::TypeId _jungfrauConfigType(Pds::TypeId::Id_JungfrauConfig,
				     JungfrauConfigType::Version);

#endif
