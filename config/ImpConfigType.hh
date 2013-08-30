#ifndef Pds_ImpConfigType_hh
#define Pds_ImpConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/imp.ddl.h"

typedef Pds::Imp::ConfigV1 ImpConfigType;

static Pds::TypeId _ImpConfigType(Pds::TypeId::Id_ImpConfig,
				  ImpConfigType::Version);

namespace Pds {
  namespace ImpConfig {
    static const unsigned NumberOfValues = ImpConfigType::NumberOfRegisters;
    unsigned       get      (const ImpConfigType&, ImpConfigType::Registers);
    void           set      (ImpConfigType&, ImpConfigType::Registers, unsigned);
    const char* const    name     (ImpConfigType::Registers);
    uint32_t       rangeHigh(ImpConfigType::Registers);
    uint32_t       rangeLow (ImpConfigType::Registers);
    uint32_t       defaultValue(ImpConfigType::Registers);
  }
}

#endif
