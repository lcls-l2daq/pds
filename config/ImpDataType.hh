#ifndef Pds_ImpDataType_hh
#define Pds_ImpDataType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/imp.ddl.h"

typedef Pds::Imp::ElementV1 ImpDataType;

static Pds::TypeId _ImpDataType(Pds::TypeId::Id_ImpData,
				  ImpDataType::Version);

namespace Pds {
  namespace ImpData {
    static const unsigned Uint32sPerHeader=8;
    static const unsigned Uint32sPerSample=2;
  }
}

#endif // Pds_ImpDataType_hh
