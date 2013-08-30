/*
 * CsPad2x2ConfigType.hh
 *
 *  Created on: Jan 10, 2012
 */

#ifndef CSPAD2x2CONFIGTYPE_HH_
#define CSPAD2x2CONFIGTYPE_HH_

#include "pds/config/CsPadConfigType.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/cspad2x2.ddl.h"

typedef Pds::CsPad2x2::ConfigV2 CsPad2x2ConfigType;
typedef Pds::CsPad2x2::ConfigV2QuadReg CsPad2x2ConfigQuadRegType;

static Pds::TypeId _CsPad2x2ConfigType(Pds::TypeId::Id_Cspad2x2Config,
                                    CsPad2x2ConfigType::Version);


namespace Pds {
  namespace CsPad2x2Config {
    typedef uint16_t GainMap[Pds::CsPad::ColumnsPerASIC][Pds::CsPad::MaxRowsPerASIC];
    void setConfig(CsPad2x2ConfigType&, 
                   const Pds::CsPad::CsPadReadOnlyCfg& readOnly,
                   unsigned concentratorVsn);
  }
}

#endif /* CSPAD2x2CONFIGTYPE_HH_ */
