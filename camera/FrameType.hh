#ifndef Pds_FrameType_hh
#define Pds_FrameType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/camera/FrameV1.hh"

typedef Pds::Camera::FrameV1 FrameType;

static Pds::TypeId _frameType(Pds::TypeId::Id_Frame,
			      FrameType::Version);

#endif
