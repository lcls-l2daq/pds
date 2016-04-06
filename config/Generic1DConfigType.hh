#ifndef Pds_Generic1DConfigType_hh
#define Pds_Generic1DConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/generic1d.ddl.h"

#include <string>

typedef Pds::Generic1D::ConfigV0 Generic1DConfigType;

static Pds::TypeId _generic1DConfigType(Pds::TypeId::Id_Generic1DConfig,
					 Generic1DConfigType::Version);


#endif
