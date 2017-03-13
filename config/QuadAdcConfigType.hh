#ifndef Pds_QuadAdcConfigType_hh
#define Pds_QuadAdcConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/quadadc.ddl.h"

typedef Pds::QuadAdc::ConfigV1 QuadAdcConfigType;

static Pds::TypeId _QuadAdcConfigType(Pds::TypeId::Id_QuadAdcConfig,
				  QuadAdcConfigType::Version);

#endif
