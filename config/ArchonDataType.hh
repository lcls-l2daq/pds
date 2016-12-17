#ifndef Pds_ArchonDataType_hh
#define Pds_ArchonDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/camera.ddl.h"

typedef Pds::Camera::FrameV1 ArchonDataType;

static Pds::TypeId _archonDataType(Pds::TypeId::Id_Frame,
          ArchonDataType::Version);

#endif
