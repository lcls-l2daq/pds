#ifndef Pds_AcqDataType_hh
#define Pds_AcqDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/acqiris.ddl.h"

typedef Pds::Acqiris::DataDescV1 AcqDataType;
typedef Pds::Acqiris::TdcDataV1 AcqTdcDataType;

static Pds::TypeId _acqDataType(Pds::TypeId::Id_AcqWaveform,
				1);
static Pds::TypeId _acqTdcDataType(Pds::TypeId::Id_AcqTdcData,
				   AcqTdcDataType::Version);
#endif
