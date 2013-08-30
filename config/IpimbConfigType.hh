#ifndef Pds_IpimbConfigType_hh
#define Pds_IpimbConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/ipimb.ddl.h"

typedef Pds::Ipimb::ConfigV2 IpimbConfigType;

static Pds::TypeId _ipimbConfigType(Pds::TypeId::Id_IpimbConfig,
				  IpimbConfigType::Version);

namespace Pds {
  namespace IpimbConfig {
    const char** cap_range();
  }
}

#endif
