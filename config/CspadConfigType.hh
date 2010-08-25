#ifndef Pds_CspadConfigType_hh
#define Pds_CspadConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/cspad/ConfigV1.hh"

typedef Pds::CsPad::ConfigV1 CspadConfigType;

static Pds::TypeId _cspadConfigType(Pds::TypeId::Id_CspadConfig,
				    CspadConfigType::Version);

#endif
