#ifndef Pds_EvrIOConfigType_hh
#define Pds_EvrIOConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/evr.ddl.h"

typedef Pds::EvrData::IOConfigV2  EvrIOConfigType;
typedef Pds::EvrData::IOChannelV2 EvrIOChannelType;

static Pds::TypeId _evrIOConfigType(Pds::TypeId::Id_EvrIOConfig,
				    EvrIOConfigType::Version);

#endif
