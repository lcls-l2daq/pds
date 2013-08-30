#ifndef Pds_FrameFexConfigType_hh
#define Pds_FrameFexConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/camera.ddl.h"

typedef Pds::Camera::FrameFexConfigV1 FrameFexConfigType;

static Pds::TypeId _frameFexConfigType(Pds::TypeId::Id_FrameFexConfig,
				       FrameFexConfigType::Version);


#endif
