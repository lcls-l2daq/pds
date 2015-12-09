#ifndef Pds_EpixDataType_hh
#define Pds_EpixDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/epix.ddl.h"

typedef Pds::Epix::ElementV1 EpixDataType;

static Pds::TypeId _epixDataType(Pds::TypeId::Id_EpixElement,
				  EpixDataType::Version);

typedef Pds::Epix::ElementV3 Epix100aDataType;

static Pds::TypeId _epix100aDataType(Pds::TypeId::Id_EpixElement,
          Epix100aDataType::Version);


#endif // Pds_EpixDataType_hh
