#ifndef Pds_EvrConfigType_hh
#define Pds_EvrConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/evr/ConfigV1.hh"

typedef Pds::EvrData::ConfigV1 EvrConfigType;

static Pds::TypeId _evrConfigType(Pds::TypeId::Id_EvrConfig,
				  EvrConfigType::Version);


#endif
