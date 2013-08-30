/*
 * CsPadConfigType.hh
 *
 *  Created on: Jun 28, 2010
 */

#ifndef CSPADCONFIGTYPE_HH_
#define CSPADCONFIGTYPE_HH_

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/cspad.ddl.h"

typedef Pds::CsPad::ConfigV5 CsPadConfigType;
typedef Pds::CsPad::ConfigV3QuadReg CspadConfigQuadRegType;

static Pds::TypeId _CsPadConfigType(Pds::TypeId::Id_CspadConfig,
                                    CsPadConfigType::Version);


namespace Pds {
  namespace CsPadConfig {
    typedef uint16_t GainMap[Pds::CsPad::ColumnsPerASIC][Pds::CsPad::MaxRowsPerASIC];
    void setConfig(CsPadConfigType&, 
                   const Pds::CsPad::CsPadReadOnlyCfg* readOnly,
                   unsigned concentratorVsn);
  }
}

#endif /* CSPADCONFIGTYPE_HH_ */
