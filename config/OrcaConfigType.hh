#ifndef Pds_OrcaConfigType_hh
#define Pds_OrcaConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/orca/ConfigV1.hh"

typedef Pds::Orca::ConfigV1 OrcaConfigType;

static Pds::TypeId _orcaConfigType(Pds::TypeId::Id_OrcaConfig,
                                   OrcaConfigType::Version);

#endif
