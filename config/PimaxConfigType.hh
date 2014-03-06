#ifndef Pds_PimaxConfigType_hh
#define Pds_PimaxConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/pimax.ddl.h"

typedef Pds::Pimax::ConfigV1 PimaxConfigType;

static Pds::TypeId _pimaxConfigType(Pds::TypeId::Id_PimaxConfig,
          PimaxConfigType::Version);

namespace Pds {
  namespace PimaxConfig {
    void setNumIntegrationShots(PimaxConfigType&,
                          unsigned);
    void setSize(PimaxConfigType&,
                 unsigned w,
                 unsigned h);
  }
}

#endif
