#ifndef Pds_PrincetonDataType_hh
#define Pds_PrincetonDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/princeton.ddl.h"

typedef Pds::Princeton::FrameV2 PrincetonDataType;
typedef Pds::Princeton::InfoV1  PrincetonInfoType;

static Pds::TypeId _princetonDataType(Pds::TypeId::Id_PrincetonFrame,
          PrincetonDataType::Version);

static Pds::TypeId _princetonInfoType(Pds::TypeId::Id_PrincetonInfo,
          PrincetonInfoType::Version);


static const float TemperatureNotDefined = -9999;

namespace Pds {
  namespace PrincetonData {
    void setTemperature(PrincetonDataType&, float);
  }
}

#endif
