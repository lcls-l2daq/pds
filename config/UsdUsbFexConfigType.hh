#ifndef UsdUsbFexConfigType_hh
#define UsdUsbFexConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/usdusb.ddl.h"

typedef Pds::UsdUsb::FexConfigV1 UsdUsbFexConfigType;

static Pds::TypeId _usdusbFexConfigType(Pds::TypeId::Id_UsdUsbFexConfig,
				     UsdUsbFexConfigType::Version);

#endif
