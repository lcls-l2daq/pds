#ifndef Pds_GenericPgpConfigType_hh
#define Pds_GenericPgpConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/genericpgp.ddl.h"

#include <string>

typedef Pds::GenericPgp::ConfigV1 GenericPgpConfigType;

static Pds::TypeId _genericPgpConfigType(Pds::TypeId::Id_GenericPgpConfig,
					 GenericPgpConfigType::Version);


#endif
