#ifndef CSPADDATATYPE_HH
#define CSPADDATATYPE_HH

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/cspad.ddl.h"

typedef Pds::CsPad::ElementV2 CsPadElementType;
typedef Pds::CsPad::DataV2 CsPadDataType;

static Pds::TypeId _CsPadDataType(Pds::TypeId::Id_CspadElement,
                                  CsPadDataType::Version);

#endif
