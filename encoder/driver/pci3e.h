#ifndef PCI3E_H
#define PCI3E_H

// We only need to include sys/ioctl.h if we're *not* building a
// module.
#ifndef LINUX_VERSION_CODE
#include <sys/ioctl.h>
#endif

#define PCI3E_VERSION "1.0"
#define PCI3E_MSG "pci3e: "

// Note that these 32-bit specific.  These all create "OR" masks (1s
// in specific positions).  Invert (~) to create AND masks (0s in
// specific positions).

// Create a mask for a specific bit.
#define mask(bitpos) (1 << bitpos)
// Create a mask for a bit AND all bits above it.
#define mask_above(bitpos) (0xFFFFFFFF << bitpos)
// Create a mask for all bits BENEATH a bit pos (not including it!).
#define mask_below(bitpos) (mask(bitpos) - 1)

// Ensure that 'val' has the same bits as 'mask' for 'len' bits
// starting at 'start'.  Assumes that 'mask' has 0s everywhere else.
#define mask_bits_into(val,start,len,mask)                                \
      (((val) & (((start) > 0) ? mask_below((start)) : 0 ))               \
    |  ((val) & ((((start)+(len)) < 32) ? mask_above((start)+(len)) : 0)) \
    | (mask))

// Channel 1 registers start at 8; channel 2 registers start at 16.
// Thus, you find a specific channel's register as this example:
// chan_2_stat = REG_STATUS + 2 * NUM_CHAN_REGS
#define GET_CHAN_REG(chan, reg)        (chan * NUM_CHAN_REGS + reg)
#define GET_CHAN_REG_OFFSET(chan, reg) (GET_CHAN_REG(chan, reg) * REG_LEN)

#define REG_TO_OFFSET(reg) (reg * REG_LEN)

// Many registers (latches, mostly) must be updated before they can be
// read.  One way to update them is to write *anything* to them -
// whatever's written is discarded.
#define JUNK (0)

/*
 * Register constants
*/

// These registers are present for each quadrature channel.  These are
// for channel 0.
#define REG_PRESET             (0)
#define REG_LATCH_OUTPUT       (1)
#define REG_MATCH              (2)
#define REG_CTRL               (3)
#define REG_STAT               (4)
#define REG_COUNTER            (5)
#define REG_TRANSFER_PRESET    (6)

#define REG_CMD                (7)
#define REG_TS_LATCH           (15)
#define REG_TS_COUNT           (23)
#define REG_TRIG_CTRL          (27)
#define REG_TRIG_STAT          (28)
#define REG_SAMPLE_RATE_MULT   (30)
#define REG_SAMPLE_CNT         (31)
#define REG_INT_CTRL           (34)
#define REG_INT_STAT           (35)
#define REG_FIFO_ENABLE        (37)
#define REG_FIFO_STAT_CTRL     (38)
#define REG_FIFO_READ          (39)
#define REG_INP_PORT           (40)
#define REG_TRIG_SETUP         (41)
#define REG_QUAL_SETUP         (42)
#define REG_NUM_SAMP           (43)
#define REG_NUM_SAMP_REMAIN    (44)
#define REG_ACQ_CTRL           (45)
#define REG_OUT_PORT           (46)
#define REG_OUT_PORT_SETUP     (47)
#define REG_LAST               (REG_OUT_PORT_SETUP+1)

// There are only 7 registers in a channel, but they're organized into
// groups that start on 8-register boundaries; there's a non-channel
// register "between" each of the channel register groups.
#   define NUM_CHAN_REGS       (8)
#   define NUM_CHAN            (3)
#   define CHAN0               (0)
#   define CHAN1               (1)
#   define CHAN2               (2)
// Register length in bytes.
#   define REG_LEN (4)

/*
 * REG_CTRL bit definitions
*/

// Bits 24-31 are reserved and will always read 0.
// Set to cause input trigger to latch counter.
#define CTRL_ENABLE_CAP (23)
// With index detection on, 0=zero counter on index, 1=write preset to
// counter on index.
#define CTRL_INDEX_PRESET_NOT_RESET (22)
// Index level.  0=Active high index, 1=active low index
#define CTRL_INDEX_INVERT (21)
// Set to write to counter on index detection (see b22).
#define CTRL_INDEX_ENABLE (20)
// If in clock/dir mode:
//    0: B=1 counts up
//    1: B=0 counts up
// If in X[124] mode:
//    0: A leads B = count up
//    1: B leads A = count up
#define CTRL_COUNT_DIR (19)
// Set to allow counter to count.
#define CTRL_ENABLE (18)
// Controls how counter counts:
#define CTRL_COUNT_MODE (16)
#define CTRL_COUNT_MODE_LEN (2)
// Controls how quadrature inputs are handled:
#define CTRL_QUAD_MODE (14)
#define CTRL_QUAD_MODE_LEN (2)
// Set to trigger when counter goes down (retards).
#define CTRL_TRIG_ON_RETARD (13)
// Set to trigger when counter goes up (advances).
#define CTRL_TRIG_ON_ADV (12)
// Set to trigger when index edge detected.
#define CTRL_TRIG_ON_INDEX (11)
// module-N mode only: Set to trigger on underflow (count down beyond 0).
#define CTRL_TRIG_ON_BORROW (10)
// module-N mode only: Set to trigger on overflow (count beyond preset).
#define CTRL_TRIG_ON_CARRY (9)
// Trigger when counter is equal to match register.
#define CTRL_TRIG_ON_MATCH (8)
// Trigger when counter is 0 (doesn't require modulo-N mode).
#define CTRL_TRIG_ON_ZERO (7)
// Bits 0-6 are reserved and will always read 0.

// CTRL_COUNT_MODE bit values
// 00 = 24-bit counter, with roll-over
// 01 = Limit counter between 0 and preset.  Can't count outside this range.
// 10 = Halt counter if outside 0 and preset.  Must reset to restart.
// 11 = module-N mode: When exceed preset, reset to 0.  When goes
//      below 0, reset to preset.
#  define COUNT_MODE_WRAP_FULL   (0)
#  define COUNT_MODE_LIMIT       (1)
#  define COUNT_MODE_HALT        (2)   
#  define COUNT_MODE_WRAP_PRESET (3)

// CTRL_QUAD_MODE bit values
// 00 = Old style behavior (A=clock, B=dir).  Every rising edge of A
//      will cause counter to change
// 01 = X1
// 10 = X2
// 11 = X4
#  define QUAD_MODE_CLOCK_DIR (0)
#  define QUAD_MODE_X1        (1)
#  define QUAD_MODE_X2        (2)
#  define QUAD_MODE_X4        (3)

/*
 * REG_STAT bit definitions.
*/
// Bits 24-31 are unused and always read 0.
#define STAT_LAST_DIR     (23)
// Bits 21-22 are unused and always read 0.
// These status bits reflect the current state of the system.
#define STAT_EVENT_RETARD (20)
#define STAT_EVENT_ADV    (19)
#define STAT_EVENT_INDEX  (18)
#define STAT_EVENT_BORROW (17)
#define STAT_EVENT_CARRY  (16)
#define STAT_EVENT_MATCH  (15)
#define STAT_EVENT_ZERO   (14)
// These status bits latch their corresponding event bits.  Once set,
// must be written with 1 to reset.
#define STAT_LATCH_RETARD (13)
#define STAT_LATCH_ADV    (12)
#define STAT_LATCH_INDEX  (11)
#define STAT_LATCH_BORROW (10)
#define STAT_LATCH_CARRY  (9)
#define STAT_LATCH_MATCH  (8)
#define STAT_LATCH_ZERO   (7)
// Bits 0-6 are unused and always read 0.

#define STAT_RESET_FLAGS (0xFFFFFFFF)

/*
 * REG_CMD: Timestamp and ROM ID register.
*/
// Read-only to return the ROM identification for the board.
#define CMD_ROM_ID (24)
#define CMD_ROM_ID_LEN (8)
// Bits 7-23 are unused and always read 0.
// Set to hold TS register at 0.  Unset to allow TS to count up.
#define CMD_TS_HALT (6)
// Set to latch the timestamp regsiter.
#define CMD_TS_LATCH (5)
// Set to latch the timestamp register *and* all counters with capture
// enabled (see b23 of CTRL).
#define CMD_LATCH_ALL (4)
// Bits 0-3 are unused and always read 0.

#define TS_HALT (1)
#define TS_RUN  (0)

/*
 * REG_INT_CTRL bit definitions.
*/
#define INT_CTRL_FIFO  (31)
#define INT_CTRL_TRIG  (30)
// Bits 0-29 are unused.

#define INT_CTRL_ENABLED  (1)
#define INT_CTRL_DISABLED (0)

/*
 * REG_INT_STAT bit definitions.
*/
#define STAT_INT_DETECTED   (31)
#define STAT_FIFO_HALF_FULL (30)
// Bits 0-29 are unused.

/*
 * REG_FIFO_ENABLE bit definitions.
*/
// Bits 9-31 are unused.
#define FIFO_ENABLE  (8)
// Bits 0-7 are unused.

#  define FIFO_ON  (1)
#  define FIFO_OFF (0)

/*
 * REG_FIFO_STAT_CTRL bit definitions.
*/
// Bits 21 to 31 are unused.
#define FIFO_STAT_DATA_CNT     (10)
#define FIFO_STAT_DATA_CNT_LEN (10)
#define FIFO_STAT_EMPTY        (9)
#define FIFO_STAT_FULL         (8)
// Bits 2-7 are unused.
#define FIFO_CTRL_INIT         (1)
#define FIFO_CTRL_UPDATE       (0)

#  define FIFO_GET_DATA_CNT(regval) \
   ((regval >> FIFO_STAT_DATA_CNT) & mask_below(FIFO_STAT_DATA_CNT_LEN))

/*
 * REG_FIFO_READ reads as a single 32-bit value.
*/

// Viewing a FIFO entry as an array, these are the meanings of each
// 32-bit value.
#define FIFO_ENTRY_LEN (5)
#define FIFO_ENTRY_TS     (0)
#define FIFO_ENTRY_CH0    (1)
#define FIFO_ENTRY_CH1    (2)
#define FIFO_ENTRY_CH2    (3)
#define FIFO_ENTRY_IOSTAT (4)

// An encoder item has two parts - the count and the status flags.
#define FIFO_ENTRY_CH_FLAGS     (24)
#define FIFO_ENTRY_CH_FLAGS_LEN (8)
#define FIFO_ENTRY_CH_CNT       (0)
#define FIFO_ENTRY_CH_CNT_LEN   (24)

#define MAX_FIFO_ENTRIES (204)
#define FIFO_ENTRY_GET_COUNT(val) (val & mask_below(FIFO_ENTRY_CH_FLAGS))
#define FIFO_ENTRY_GET_FLAGS(val) \
   ((val & mask_above(FIFO_ENTRY_CH_FLAGS)) >> FIFO_ENTRY_CH_CNT_LEN)

/*
 * REG_TRIG_CTRL bit definitions.
*/
// Bits 8-31 are unused.
#define TRIG_CTRL_INVERT3  (7)
#define TRIG_CTRL_INVERT2  (6)
#define TRIG_CTRL_INVERT1  (5)
#define TRIG_CTRL_INVERT0  (4)
#define TRIG_CTRL_ENABLE3  (3)
#define TRIG_CTRL_ENABLE2  (2)
#define TRIG_CTRL_ENABLE1  (1)
#define TRIG_CTRL_ENABLE0  (0)
#   define TRIG_CTRL_RISING  (1)
#   define TRIG_CTRL_FALLING (0)
#   define TRIG_CTRL_ENABLE  (1)
#   define TRIG_CTRL_DISABLE (0)

/*
 * REG_TRIG_STAT bit definitions.
*/
// Bits 4-31 are unused.
#define TRIG_DET_IN3  (3)
#define TRIG_DET_IN2  (2)
#define TRIG_DET_IN1  (1)
#define TRIG_DET_IN0  (0)
#   define TRIG_DET_CLEAR (1)

/*
 * REG_INP_PORT bit definitions.
*/
// Bits 4-31 are unused.
#define INP_IN3
#define INP_IN2
#define INP_IN1
#define INP_IN0
#   define INP_LO (1)
#   define INP_HI (0)

/*
 * REG_TRIG_SETUP
*/
#define TRIG_AND_OR (31)
// Bits 12-30 are unused.
#define TRIG_SETUP_IN3 (9)
#define TRIG_SETUP_IN2 (6)
#define TRIG_SETUP_IN1 (3)
#define TRIG_SETUP_IN0 (0)

/*
 * REG_QUAL_SETUP
*/
#define QUAL_AND_OR (31)
// Bits 12-30 are unused.
#define QUAL_SETUP_IN3 (9)
#define QUAL_SETUP_IN2 (6)
#define QUAL_SETUP_IN1 (3)
#define QUAL_SETUP_IN0 (0)

/*
 * Values for REG_{QUAL,TRIG}_SETUP.
*/
#define TRIG_QUAL_AND (1)
#define TRIG_QUAL_OR  (0)
#define TRIG_QUAL_SETUP_LEN          (3)
#define TRIG_QUAL_SETUP_IGNORE       (0)
#define TRIG_QUAL_SETUP_RISING_EDGE  (1)
#define TRIG_QUAL_SETUP_FALLING_EDGE (2)
#define TRIG_QUAL_SETUP_EDGE         (3)
#define TRIG_QUAL_SETUP_HIGH         (4)
#define TRIG_QUAL_SETUP_LOW          (5)     
#define TRIG_QUAL_SETUP_ALWAYS       (6)

#define TRIG_QUAL_ALL_OFF (0x00000000)

/*
 * REG_ACQ_CTRL bit definitions.
*/

// Bits 3-31 are unused.
#define ACQ_CONTINOUS_MODE  (2)
#define ACQ_TRIG_MODE       (1)
#define ACQ_ENABLE          (0)

#define ACQ_ALL_OFF (0x00000000)

/*
 * REG_OUT_PORT bit definitions.
*/
// Bits 4-31 are unused.
#define OUT_OUT3
#define OUT_OUT2
#define OUT_OUT1
#define OUT_OUT0
#   define OUT_ON  (1)
#   define OUT_OFF (0)

/*
 * REG_OUT_PORT_SETUP bit meanings.
*/
#define OUT_TRIG_DUR (30)
#   define OUT_TRIG_LEN (2)
#   define OUT_TRIG_DUR_1MS   (0)
#   define OUT_TRIG_DUR_200uS (1)
#   define OUT_TRIG_DUR_20uS  (2)
#   define OUT_TRIG_DUR_5uS   (3)
// Bits 4-29 are unused.
#define OUT3_SOURCE (3)
#define OUT2_SOURCE (2)
#define OUT1_SOURCE (1)
#define OUT0_SOURCE (0)
#   define OUTN_SOURCE_USER (0)
#   define OUTN_SOURCE_TRIG_OUT (1)
#   define OUT3_SOURCE_COMBINED_TRIG_OUT (1)

/* this is used by ioctl() commands */
struct pci3e_io_struct {
    unsigned long offset; /* within the region */
    unsigned long value;   /* read or written */
};

/*
 * ioctl() commands
*/
#define PCI3E_IOC_MAGIC  'p' /* Use 'p' as magic number */
#define IOCREAD       _IOWR(PCI3E_IOC_MAGIC, 1, struct pci3e_io_struct)
#define IOCWRITE      _IOW( PCI3E_IOC_MAGIC, 2, struct pci3e_io_struct)
#define IOCDISABLEIRQ _IOWR(PCI3E_IOC_MAGIC, 3, struct pci3e_io_struct)
#define IOCREADFIFO   _IOWR(PCI3E_IOC_MAGIC, 4, uint32_t*)


// Here are the same register definitions, but represented as
// bitfields.

union REG_CTRL_t
{
   uint32_t whole;
   struct
   {
      unsigned reserved1 : 7;
      unsigned trig_on_zero : 1;
      unsigned trig_on_match : 1;
      unsigned trig_on_carry : 1;
      unsigned trig_on_borrow : 1;
      unsigned trig_on_index : 1;
      unsigned trig_on_advance : 1;
      unsigned trig_on_retard : 1;
      unsigned quad_mode : 2;
      unsigned count_mode : 2;
      unsigned enable : 1;
      unsigned count_dir : 1;
      unsigned index_enable : 1;
      unsigned index_invert : 1;
      unsigned index_copies_preset : 1;
      unsigned enable_capture : 1;
      unsigned reserved2 : 8;
   };
};

union REG_FIFO_STAT_CTRL_t
{
   uint32_t whole;
   struct
   {
      unsigned update : 1;
      unsigned init : 1;
      unsigned reserved1 : 6;
      unsigned is_full : 1;
      unsigned is_empty : 1;
      unsigned num_items : 11;
      unsigned reserved2 : 11;
   };
};

union REG_FIFO_ENABLE_t
{
   uint32_t whole;
   struct
   {
      unsigned reserved1 : 8;
      unsigned enable : 1;
      unsigned reserved2 : 23;
   };
};

union REG_TRIG_CTRL_t
{
   uint32_t whole;
   struct
   {
      unsigned enable0 : 1;
      unsigned enable1 : 1;
      unsigned enable2 : 1;
      unsigned enable3 : 1;
      unsigned invert0 : 1;
      unsigned invert1 : 1;
      unsigned invert2 : 1;
      unsigned invert3 : 1;
      unsigned reserved : 24;
   };
};

union REG_INT_CTRL_t
{
   uint32_t whole;
   struct
   {
      unsigned reserved : 30;
      unsigned on_trig : 1;
      unsigned on_fifo : 1;
   };
};

union REG_CMD_t
{
   uint32_t whole;
   struct
   {
      unsigned reserved1 : 4;
      unsigned latch_all_channels : 1;
      unsigned latch_timestamp : 1;
      unsigned stop_timestamp : 1;
      unsigned reserved2 : 17;
      unsigned rom_id : 8;
   };
};

#endif
