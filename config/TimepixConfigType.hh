#ifndef Pds_TimepixConfigType_hh
#define Pds_TimepixConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/timepix/ConfigV1.hh"

typedef Pds::Timepix::ConfigV1 TimepixConfigType;

static Pds::TypeId _timepixConfigType(Pds::TypeId::Id_TimepixConfig,
                                      TimepixConfigType::Version);

#endif
