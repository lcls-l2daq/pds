#ifndef ENCODERDATATYPE_HH
#define ENCODERDATATYPE_HH

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/encoder.ddl.h"

typedef Pds::Encoder::DataV2 EncoderDataType;

static Pds::TypeId _encoderDataType(Pds::TypeId::Id_EncoderData,
                                    EncoderDataType::Version);

#endif
