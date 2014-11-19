#ifndef Pds_TimeToolConfigType_hh
#define Pds_TimeToolConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/timetool.ddl.h"

typedef Pds::TimeTool::ConfigV2  TimeToolConfigType;

static Pds::TypeId _timetoolConfigType(Pds::TypeId::Id_TimeToolConfig,
				       TimeToolConfigType::Version);

namespace Pds {
  namespace TimeTool {
    void dump(const TimeToolConfigType&);
  };
};
    
#endif
