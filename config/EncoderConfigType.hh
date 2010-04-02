#ifndef ENCODERCONFIGTYPE_HH
#define ENCODERCONFIGTYPE_HH

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/encoder/ConfigV1.hh"

typedef Pds::Encoder::ConfigV1 EncoderConfigType;

static Pds::TypeId _encoderConfigType(Pds::TypeId::Id_EncoderConfig,
                                      EncoderConfigType::Version);

#endif
