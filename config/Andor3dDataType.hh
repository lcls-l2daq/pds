#ifndef Pds_Andor3dDataType_hh
#define Pds_Andor3dDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/andor3d.ddl.h"

typedef Pds::Andor3d::FrameV1 Andor3dDataType;

static Pds::TypeId _andor3dDataType(Pds::TypeId::Id_Andor3dFrame,
          Andor3dDataType::Version);

#endif
