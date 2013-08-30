#ifndef Pds_FrameType_hh
#define Pds_FrameType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/camera.ddl.h"

typedef Pds::Camera::FrameV1 FrameType;

static Pds::TypeId _frameType(Pds::TypeId::Id_Frame,
			      FrameType::Version);

#endif
