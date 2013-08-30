#ifndef Pds_FccdConfigType_hh
#define Pds_FccdConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/fccd.ddl.h"

typedef Pds::FCCD::FccdConfigV2 FccdConfigType;

static Pds::TypeId _fccdConfigType(Pds::TypeId::Id_FccdConfig,
				     FccdConfigType::Version);

typedef struct {
  float       v_start;
  float       v_end;
  const char* label;
} dac_setup;

static const dac_setup FCCD_DAC[] = { {  0.0,  9.0, "V(1-3) pos (0.0->9.0)" },
                                      {  0.0, -9.0, "V(1-3) neg (-9.0->0.0)" },
                                      {  0.0,  9.0, "TgClk pos (0.0->9.0)" },
                                      {  0.0, -9.0, "TgClk neg (-9.0->0.0)" },
                                      {  0.0,  9.0, "H(1-3) pos (0.0->9.0)" },
                                      {  0.0, -9.0, "H(1-3) neg (-9.0->0.0)" },
                                      {  0.0,  9.0, "SwtClk pos (0.0->9.0)" },
                                      {  0.0, -9.0, "SwtClk neg (-9.0->0.0)" },
                                      {  0.0,  9.0, "RgClk pos (0.0->9.0)" },
                                      {  0.0, -9.0, "RgClk neg (-9.0->0.0)" },
                                      {  0.0, 99.0, "HV (0.0->99.0)" },
                                      {  0.0,  5.0, "OTG (0.0->5.0)" },
                                      {  0.0,-15.0, "VDDRST (0.0->-15.0)" },
                                      {  0.0,-24.0, "VDDOUT (0.0->-24.0)" },
                                      {-10.0, 10.0, "NGD (-10.0->10.0)" },
                                      {-10.0, 10.0, "NCON (-10.0->10.0)" },
                                      {-10.0, 10.0, "GAURD (-10.0->10.0)" } };

#endif
