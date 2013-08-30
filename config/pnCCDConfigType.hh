#ifndef Pds_pnCCDConfigType_hh
#define Pds_pnCCDConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/pnccd.ddl.h"

#include <string>

typedef Pds::PNCCD::ConfigV2 pnCCDConfigType;

static Pds::TypeId _pnCCDConfigType(Pds::TypeId::Id_pnCCDconfig,
                                    pnCCDConfigType::Version);


namespace Pds {
  namespace pnCCDConfig {
    void setNumLinks( pnCCDConfigType&, const std::string& );
  }
}

#endif
