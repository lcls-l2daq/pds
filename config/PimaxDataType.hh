#ifndef Pds_PimaxDataType_hh
#define Pds_PimaxDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/pimax.ddl.h"

typedef Pds::Pimax::FrameV1 PimaxDataType;

static Pds::TypeId _pimaxDataType(Pds::TypeId::Id_PimaxFrame,
          PimaxDataType::Version);

namespace Pds {
  namespace PimaxData {
    void setTemperature(PimaxDataType&,
                        float);
  }
}

#endif
