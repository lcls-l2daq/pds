#ifndef Pds_TimepixConfigType_hh
#define Pds_TimepixConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/timepix.ddl.h"

typedef Pds::Timepix::ConfigV3 TimepixConfigType;

static Pds::TypeId _timepixConfigType(Pds::TypeId::Id_TimepixConfig,
                                      TimepixConfigType::Version);

namespace Pds {
  namespace TimepixConfig {
    void setConfig(TimepixConfigType&,
                   const ndarray<const std::string,1>& chipNames,
                   const ndarray<const uint32_t   ,1>& chipIDs,
                   int                                 driverVersion,
                   unsigned                            firmwareVersion,
                   unsigned                            nthresh = 0,
                   const uint8_t*                      threshs = NULL);
  }
}

#endif
