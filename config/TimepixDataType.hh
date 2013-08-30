#ifndef TIMEPIXDATATYPE_HH
#define TIMEPIXDATATYPE_HH

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/timepix.ddl.h"

typedef Pds::Timepix::DataV2 TimepixDataType;

static Pds::TypeId _timepixDataType(Pds::TypeId::Id_TimepixData,
                                    TimepixDataType::Version);

namespace Pds {
  namespace TimepixData {
    static const unsigned RawDataBytes = 
      Pds::Timepix::DataV1::Height*
      Pds::Timepix::DataV1::Width*
      Pds::Timepix::DataV1::Depth/8;
    static const unsigned DecodedDataBytes = 
      Pds::Timepix::DataV1::Height*
      Pds::Timepix::DataV1::Width*
      Pds::Timepix::DataV1::DepthBytes;
  }
}

#endif
