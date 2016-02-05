#ifndef Pds_Andor3dConfigType_hh
#define Pds_Andor3dConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/andor3d.ddl.h"

typedef Pds::Andor3d::ConfigV1 Andor3dConfigType;

static Pds::TypeId _andor3dConfigType(Pds::TypeId::Id_Andor3dConfig,
          Andor3dConfigType::Version);

namespace Pds {
  namespace Andor3dConfig {
    void setNumDelayShots(Andor3dConfigType&,
                          unsigned);
    void setSize(Andor3dConfigType&,
                 unsigned w,
                 unsigned h);
    void setNumSensors(Andor3dConfigType&,
                       unsigned s);
  }
}

#endif
