/*
 * CsPadConfigType.hh
 *
 *  Created on: Jun 28, 2010
 */

#ifndef CSPADCONFIGTYPE_HH_
#define CSPADCONFIGTYPE_HH_

typedef Pds::CsPad::ConfigV1 CsPadConfigType;

static Pds::TypeId _CsPadConfigType(Pds::TypeId::Id_CspadConfig,
                                    CsPadConfigType::Version);


#endif /* CSPADCONFIGTYPE_HH_ */
