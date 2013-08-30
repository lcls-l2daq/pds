#ifndef Pds_TwoDGaussianType_hh
#define Pds_TwoDGaussianType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/camera.ddl.h"

typedef Pds::Camera::TwoDGaussianV1 TwoDGaussianType;

static Pds::TypeId _twoDGType(Pds::TypeId::Id_TwoDGaussian,
			      TwoDGaussianType::Version);

#endif
