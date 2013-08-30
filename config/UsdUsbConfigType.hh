#ifndef UsdUsbConfigType_hh
#define UsdUsbConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/usdusb.ddl.h"

typedef Pds::UsdUsb::ConfigV1 UsdUsbConfigType;

static Pds::TypeId _usdusbConfigType(Pds::TypeId::Id_UsdUsbConfig,
				     UsdUsbConfigType::Version);

#endif
