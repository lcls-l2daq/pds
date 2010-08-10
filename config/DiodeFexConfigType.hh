#ifndef Pds_DiodeFexConfigType_hh
#define Pds_DiodeFexConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/lusi/DiodeFexConfigV1.hh"

typedef Pds::Lusi::DiodeFexConfigV1 DiodeFexConfigType;

static Pds::TypeId _diodeFexConfigType(Pds::TypeId::Id_DiodeFexConfig,
				       DiodeFexConfigType::Version);

#endif
