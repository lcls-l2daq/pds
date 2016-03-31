#ifndef UsdUsbFexDataType_hh
#define UsdUsbFexDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/usdusb.ddl.h"

typedef Pds::UsdUsb::FexDataV1 UsdUsbFexDataType;

static Pds::TypeId _usdusbFexDataType(Pds::TypeId::Id_UsdUsbFexData,
				   UsdUsbFexDataType::Version);

#endif
