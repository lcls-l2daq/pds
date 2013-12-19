#ifndef Pds_EpixConfigType_hh
#define Pds_EpixConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/epix.ddl.h"
#include "pds/config/EpixConfigV1.hh"
#include "pds/config/EpixASICConfigV1.hh"

#include <string>

typedef Pds::Epix::ConfigV1 EpixConfigType;
typedef Pds::EpixConfig::ConfigV1 EpixConfigShadow;
typedef Pds::EpixConfig::ASIC_ConfigV1 EpixASIC_ConfigShadow;

static Pds::TypeId _epixConfigType(Pds::TypeId::Id_EpixConfig,
                                    EpixConfigType::Version);

#endif
