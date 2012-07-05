#ifndef Pds_QuartzConfigType_hh
#define Pds_QuartzConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/quartz/ConfigV1.hh"

typedef Pds::Quartz::ConfigV1 QuartzConfigType;

static Pds::TypeId _quartzConfigType(Pds::TypeId::Id_QuartzConfig,
				     QuartzConfigType::Version);

#endif
