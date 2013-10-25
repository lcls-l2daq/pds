// $Id$
// Author: Chris Ford <caf@slac.stanford.edu>

#ifndef __RAYONIX_COMMON_HH
#define __RAYONIX_COMMON_HH

#include <stdint.h>

#define RNX_IPADDR          "10.0.1.101"
#define RNX_DATA_PORT_EVEN  30000
#define RNX_DATA_PORT_ODD   30001
#define RNX_CONTROL_PORT    30042
#define RNX_NOTIFY_PORT     30050
#define RNX_SIM_TRIGGER_PORT 30051

#define MAX_LINE_PIXELS     3840
#define MAX_FRAME_PIXELS    (MAX_LINE_PIXELS * MAX_LINE_PIXELS)

typedef struct {
  uint32_t  cmd;
  uint16_t  frameNumber;
  uint16_t  lineNumber;
  uint32_t  damage;
  uint32_t  epoch;
  uint8_t   binning_f;
  uint8_t   binning_s;
  uint16_t  pad;
} data_footer_t;

#endif
