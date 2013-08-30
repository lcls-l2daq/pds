#ifndef UsdUsbDataType_hh
#define UsdUsbDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/usdusb.ddl.h"

typedef Pds::UsdUsb::DataV1 UsdUsbDataType;

static Pds::TypeId _usdusbDataType(Pds::TypeId::Id_UsdUsbData,
				   UsdUsbDataType::Version);

#endif
