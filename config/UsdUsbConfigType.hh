#ifndef UsdUsbConfigType_hh
#define UsdUsbConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/usdusb/ConfigV1.hh"

typedef Pds::UsdUsb::ConfigV1 UsdUsbConfigType;

static Pds::TypeId _usdusbConfigType(Pds::TypeId::Id_UsdUsbConfig,
				     UsdUsbConfigType::Version);

#endif
