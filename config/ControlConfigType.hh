#ifndef Pds_ControlConfigType_hh
#define Pds_ControlConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/control/ConfigV2.hh"

typedef Pds::ControlData::ConfigV2 ControlConfigType;

static Pds::TypeId _controlConfigType(Pds::TypeId::Id_ControlConfig,
				      ControlConfigType::Version);


#endif
