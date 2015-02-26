#ifndef Pds_EvsConfigType_hh
#define Pds_EvsConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/evr.ddl.h"

typedef Pds::EvrData::SrcConfigV1 EvsConfigType;
typedef Pds::EvrData::DataV4      EvrDataType;

typedef Pds::EvrData::SrcEventCode      EvsCodeType;  
typedef Pds::EvrData::PulseConfigV3     PulseType;  
typedef Pds::EvrData::OutputMapV2       OutputMapType;  

static Pds::TypeId _evsConfigType(Pds::TypeId::Id_EvsConfig,
				  EvsConfigType::Version);

namespace Pds {
  namespace EvsConfig {
#if 0
    void clearMask(EvsCodeType&, unsigned maskType);
    void setMask  (EvsCodeType&, unsigned maskType, unsigned maskValue);
#endif
    unsigned size(const EvsConfigType&);

    unsigned map(const OutputMapType&);
  }
}

#endif
