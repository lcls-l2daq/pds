#ifndef Pds_RayonixConfigType_hh
#define Pds_RayonixConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/rayonix.ddl.h"

typedef Pds::Rayonix::ConfigV2 RayonixConfigType;

static Pds::TypeId _rayonixConfigType(Pds::TypeId::Id_RayonixConfig,
                                      RayonixConfigType::Version);

namespace Pds {
  namespace RayonixConfig {
    void setConfig(RayonixConfigType&);
  }
}

#endif
