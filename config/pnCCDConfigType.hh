#ifndef Pds_pnCCDConfigType_hh
#define Pds_pnCCDConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/pnCCD/ConfigV1.hh"

typedef Pds::pnCCDData::ConfigV1 pnCCDConfigType;

static Pds::TypeId _pnCCDConfigType(Pds::TypeId::Id_pnCCDconfig,
                                    pnCCDConfigType::Version);


#endif
