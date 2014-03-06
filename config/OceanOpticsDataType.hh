#ifndef Pds_OceanOpticsDataType_hh
#define Pds_OceanOpticsDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/oceanoptics.ddl.h"

typedef Pds::OceanOptics::DataV1 OOpt_HR4000_DataType;
typedef Pds::OceanOptics::DataV2 OOpt_USB2000P_DataType;

static Pds::TypeId _oopt_HR4000_DataType(Pds::TypeId::Id_OceanOpticsData,
          OOpt_HR4000_DataType::Version);
static Pds::TypeId _oopt_USB2000P_DataType(Pds::TypeId::Id_OceanOpticsData,
          OOpt_USB2000P_DataType::Version);


#endif
