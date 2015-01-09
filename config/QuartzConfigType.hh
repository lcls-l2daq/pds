#ifndef Pds_QuartzConfigType_hh
#define Pds_QuartzConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/quartz.ddl.h"

typedef Pds::Quartz::ConfigV2 QuartzConfigType;

static Pds::TypeId _quartzConfigType(Pds::TypeId::Id_QuartzConfig,
				     QuartzConfigType::Version);

namespace Pds {
  class DetInfo;
  namespace Quartz {
    unsigned max_row_pixels   (const DetInfo&);
    unsigned max_column_pixels(const DetInfo&);
  }
};

#endif
