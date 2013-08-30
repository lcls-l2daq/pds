#ifndef ENCODERCONFIGTYPE_HH
#define ENCODERCONFIGTYPE_HH

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/encoder.ddl.h"

typedef Pds::Encoder::ConfigV2 EncoderConfigType;

static Pds::TypeId _encoderConfigType(Pds::TypeId::Id_EncoderConfig,
                                      EncoderConfigType::Version);

#endif
