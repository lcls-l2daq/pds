#ifndef Pds_Generic1DDataType_hh
#define Pds_Generic1DDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/generic1d.ddl.h"

typedef Pds::Generic1D::DataV0 Generic1DDataType;

static Pds::TypeId _generic1DDataType(Pds::TypeId::Id_Generic1DData,
                                      Generic1DDataType::Version);

#endif
