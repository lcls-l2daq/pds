#ifndef Pds_EpixConfigType_hh
#define Pds_EpixConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/epix.ddl.h"
#include "pds/config/EpixConfigV1.hh"
#include "pds/config/EpixASICConfigV1.hh"
#include "pds/config/Epix10kConfigV1.hh"
#include "pds/config/Epix10kASICConfigV1.hh"

#include <string>

typedef Pds::Epix::ConfigV1 EpixConfigType;
typedef Pds::EpixConfig::ConfigV1 EpixConfigShadow;
typedef Pds::EpixConfig::ASIC_ConfigV1 EpixASIC_ConfigShadow;

static Pds::TypeId _epixConfigType(Pds::TypeId::Id_EpixConfig,
                                    EpixConfigType::Version);

typedef Pds::Epix::Config10KV1 Epix10kConfigType;
typedef Pds::Epix10kConfig::ConfigV1 Epix10kConfigShadow;
typedef Pds::Epix10kConfig::ASIC_ConfigV1 Epix10kASIC_ConfigShadow;

static Pds::TypeId _epix10kConfigType(Pds::TypeId::Id_Epix10kConfig,
                                    Epix10kConfigType::Version);

#endif
