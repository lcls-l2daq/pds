#ifndef Pds_FliConfigType_hh
#define Pds_FliConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/fli.ddl.h"

typedef Pds::Fli::ConfigV1 FliConfigType;

static Pds::TypeId _fliConfigType(Pds::TypeId::Id_FliConfig,
                                  FliConfigType::Version);

namespace Pds {
  namespace FliConfig {
    void setNumDelayShots(FliConfigType&,
                          unsigned);
    void setSize(FliConfigType&,
                 unsigned w,
                 unsigned h);
  }
}

#endif
