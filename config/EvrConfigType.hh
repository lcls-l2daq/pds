#ifndef Pds_EvrConfigType_hh
#define Pds_EvrConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/evr/ConfigV2.hh"
#include "pdsdata/evr/PulseConfig.hh"
#include "pdsdata/evr/OutputMap.hh"

typedef Pds::EvrData::ConfigV2 EvrConfigType;

static Pds::TypeId _evrConfigType(Pds::TypeId::Id_EvrConfig,
				  EvrConfigType::Version);


#endif
