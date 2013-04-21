#ifndef Pds_ImpDataType_hh
#define Pds_ImpDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/imp/ElementV1.hh"

typedef Pds::Imp::ElementV1 ImpDataType;

static Pds::TypeId _ImpDataType(Pds::TypeId::Id_ImpData,
				  ImpDataType::Version);

#endif // Pds_ImpDataType_hh
