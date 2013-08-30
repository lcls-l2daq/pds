#ifndef Pds_PrincetonConfigType_hh
#define Pds_PrincetonConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/princeton.ddl.h"

typedef Pds::Princeton::ConfigV5 PrincetonConfigType;

static Pds::TypeId _princetonConfigType(Pds::TypeId::Id_PrincetonConfig,
          PrincetonConfigType::Version);

namespace Pds {
  namespace PrincetonConfig {
    void setNumDelayShots(PrincetonConfigType&, unsigned);
    void setWidth        (PrincetonConfigType&, unsigned);
    void setHeight       (PrincetonConfigType&, unsigned);
    void setSize         (PrincetonConfigType&, unsigned w, unsigned h);
    void setReadoutSpeedIndex(PrincetonConfigType&, unsigned index);
  }
}

#endif
