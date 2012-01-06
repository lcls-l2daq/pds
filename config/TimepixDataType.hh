#ifndef TIMEPIXDATATYPE_HH
#define TIMEPIXDATATYPE_HH

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/timepix/DataV1.hh"

typedef Pds::Timepix::DataV1 TimepixDataType;

static Pds::TypeId _timepixDataType(Pds::TypeId::Id_TimepixData,
                                    TimepixDataType::Version);

#endif
