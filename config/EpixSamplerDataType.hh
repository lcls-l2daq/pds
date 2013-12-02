#ifndef Pds_EpixSamplerDataType_hh
#define Pds_EpixSamplerDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/epixsampler.ddl.h"

typedef Pds::EpixSampler::ElementV1 EpixSamplerDataType;

static Pds::TypeId _epixSamplerDataType(Pds::TypeId::Id_EpixSamplerElement,
				  EpixSamplerDataType::Version);


#endif // Pds_EpixSamplerDataType_hh
