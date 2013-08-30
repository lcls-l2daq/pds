#ifndef Pds_EvrConfigType_hh
#define Pds_EvrConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/evr.ddl.h"

typedef Pds::EvrData::ConfigV7  EvrConfigType;
typedef Pds::EvrData::DataV3    EvrDataType;

typedef Pds::EvrData::EventCodeV6       EventCodeType;  
typedef Pds::EvrData::PulseConfigV3     PulseType;  
typedef Pds::EvrData::OutputMapV2       OutputMapType;  
typedef Pds::EvrData::SequencerConfigV1 SeqConfigType;

static Pds::TypeId _evrConfigType(Pds::TypeId::Id_EvrConfig,
          EvrConfigType::Version);

namespace Pds {
  namespace EvrConfig {
    void clearMask(EventCodeType&, unsigned maskType);
    void setMask  (EventCodeType&, unsigned maskType, unsigned maskValue);

    unsigned size(const EvrConfigType&);

    unsigned map(const OutputMapType&);
  }
}

#endif
