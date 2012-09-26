#ifndef Pds_AndorConfigType_hh
#define Pds_AndorConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/andor/ConfigV1.hh"

typedef Pds::Andor::ConfigV1 AndorConfigType;

static Pds::TypeId _andorConfigType(Pds::TypeId::Id_AndorConfig,
          AndorConfigType::Version);

#endif
