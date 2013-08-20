#ifndef Pds_AliasConfigType_hh
#define Pds_AliasConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/alias/ConfigV1.hh"

typedef Pds::Alias::ConfigV1 AliasConfigType;

static Pds::TypeId _aliasConfigType(Pds::TypeId::Id_AliasConfig,
                                      AliasConfigType::Version);

#endif
