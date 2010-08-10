#ifndef Pds_IpmFexConfigType_hh
#define Pds_IpmFexConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/lusi/IpmFexConfigV1.hh"

typedef Pds::Lusi::IpmFexConfigV1 IpmFexConfigType;

static Pds::TypeId _ipmFexConfigType(Pds::TypeId::Id_IpmFexConfig,
				     IpmFexConfigType::Version);

#endif
