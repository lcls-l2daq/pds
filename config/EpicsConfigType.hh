#ifndef Pds_EpicsConfigType_hh
#define Pds_EpicsConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/epics.ddl.h"

typedef Pds::Epics::ConfigV1 EpicsConfigType;

static Pds::TypeId _epicsConfigType(Pds::TypeId::Id_EpicsConfig,
          EpicsConfigType::Version);

namespace Pds {
  namespace EpicsConfig {
    typedef Pds::Epics::PvConfigV1 PvConfigType;
  }
}

#endif
