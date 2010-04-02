#ifndef ENCODERDATATYPE_HH
#define ENCODERDATATYPE_HH

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/encoder/DataV1.hh"

typedef Pds::Encoder::DataV1 EncoderDataType;

static Pds::TypeId _encoderDataType(Pds::TypeId::Id_EncoderData,
                                    EncoderDataType::Version);

#endif
