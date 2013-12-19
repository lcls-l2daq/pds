#ifndef Pds_EpixDataType_hh
#define Pds_EpixDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/epix.ddl.h"

typedef Pds::Epix::ElementV1 EpixDataType;

static Pds::TypeId _epixDataType(Pds::TypeId::Id_EpixElement,
				  EpixDataType::Version);


#endif // Pds_EpixDataType_hh
