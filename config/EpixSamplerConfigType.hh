#ifndef Pds_EpixSamplerConfigType_hh
#define Pds_EpixSamplerConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/epixsampler.ddl.h"
#include "pds/config/EpixSamplerConfigV1.hh"

#include <string>

typedef Pds::EpixSampler::ConfigV1 EpixSamplerConfigType;
typedef Pds::EpixSamplerConfig::ConfigV1 EpixSamplerConfigShadow;

static Pds::TypeId _epixSamplerConfigType(Pds::TypeId::Id_EpixSamplerConfig,
                                    EpixSamplerConfigType::Version);

#endif
