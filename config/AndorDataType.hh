#ifndef Pds_AndorDataType_hh
#define Pds_AndorDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/andor.ddl.h"

typedef Pds::Andor::FrameV1 AndorDataType;

static Pds::TypeId _andorDataType(Pds::TypeId::Id_AndorFrame,
          AndorDataType::Version);

namespace Pds {
  namespace AndorData {
    void setTemperature(AndorDataType&,
                        float);
  }
}

#endif
