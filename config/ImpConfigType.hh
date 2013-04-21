#ifndef Pds_ImpConfigType_hh
#define Pds_ImpConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/imp/ConfigV1.hh"

typedef Pds::Imp::ConfigV1 ImpConfigType;

static Pds::TypeId _ImpConfigType(Pds::TypeId::Id_ImpConfig,
				  ImpConfigType::Version);

#endif
