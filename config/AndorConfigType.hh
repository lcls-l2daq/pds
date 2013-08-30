#ifndef Pds_AndorConfigType_hh
#define Pds_AndorConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/andor.ddl.h"

typedef Pds::Andor::ConfigV1 AndorConfigType;

static Pds::TypeId _andorConfigType(Pds::TypeId::Id_AndorConfig,
          AndorConfigType::Version);

namespace Pds {
  namespace AndorConfig {
    void setNumDelayShots(AndorConfigType&,
                          unsigned);
    void setSize(AndorConfigType&,
                 unsigned w,
                 unsigned h);
  }
}

#endif
