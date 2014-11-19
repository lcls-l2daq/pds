#ifndef Pds_TimeToolDataType_hh
#define Pds_TimeToolDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/timetool.ddl.h"

typedef Pds::TimeTool::DataV2  TimeToolDataType;

static Pds::TypeId _timetoolDataType(Pds::TypeId::Id_TimeToolConfig,
                                     TimeToolDataType::Version);

#endif
