#ifndef Pds_Opal1kConfigType_hh
#define Pds_Opal1kConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/opal1k.ddl.h"

typedef Pds::Opal1k::ConfigV1 Opal1kConfigType;

static Pds::TypeId _opal1kConfigType(Pds::TypeId::Id_Opal1kConfig,
				     Opal1kConfigType::Version);

namespace Pds {
  class DetInfo;
  namespace Opal1k {
    unsigned max_row_pixels   (const DetInfo&);
    unsigned max_column_pixels(const DetInfo&);
  }
};

#endif
