#ifndef Pds_AcqConfigType_hh
#define Pds_AcqConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/acqiris/ConfigV1.hh"

typedef Pds::Acqiris::ConfigV1 AcqConfigType;

static Pds::TypeId _acqConfigType(Pds::TypeId::Id_AcqConfig,
				  AcqConfigType::Version);

#endif
