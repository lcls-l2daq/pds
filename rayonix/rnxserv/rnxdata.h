#ifndef _RNX_DATA_H
#define _RNX_DATA_H

//#define RNX_DATA_PORT     30000
#define RNX_DATA_PORT_EVEN  30000
#define RNX_DATA_PORT_ODD   30001
#define RNX_NOTIFY_PORT     30050
#define RNX_TRIGGER_PORT    30051 /* sim */

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

#endif /* _RNX_DATA_H */
