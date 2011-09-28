// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_common.h $
// $Rev: 9240 $
// $Date$

#ifndef __GSC_COMMON_H__
#define __GSC_COMMON_H__



// #defines	*******************************************************************

#define GSC_IOCTL					's'


// Register definition macros

#define	GSC_REG_ALT					0xC000
#define	GSC_REG_PCI					0x8000
#define	GSC_REG_PLX					0x4000
#define	GSC_REG_GSC					0x0000

//	Encoding and decoding of registers.
//	r = register value to decode
//	v = field value to encode
//	b = beginning bit (most significant bit position, 31 is the upper most bit).
//	e = ending bit (least significant bit position, 0 is the lower most bit).
#define	GSC_FIELD_DECODE(r,b,e)		(((r)>>(e)) & (0xFFFFFFFF >> (31-((b)-(e)))))
#define	GSC_FIELD_ENCODE(v,b,e)		(((v) << (e)) & ((0xFFFFFFFF << (e)) & (0xFFFFFFFF >> (31 - (b)))))

//	Register definitions:
//	bits 15-14:	register type (PCI, PLX, PLDA, or GSC product specific)
//	bits 13-12:	register size (1, 2 or 4 bytes)
//	bits 11-0:	register offset (0 to 0xFFF)
#define	GSC_REG_ENCODE(t,s,o)		(((t) & 0xC000) | \
									GSC_FIELD_ENCODE((s) - 1,13,12) | \
									GSC_FIELD_ENCODE((o),11,0))

#define	GSC_REG_OFFSET(r)			GSC_FIELD_DECODE((r),11,0)
#define	GSC_REG_SIZE(r)				(GSC_FIELD_DECODE((r),13,12) + 1)
#define	GSC_REG_TYPE(r)				((r) & 0xC000)

// Wait macros: gsc_wait_t.flags
#define	GSC_WAIT_FLAG_CANCEL			0x0001	// a wait was cancelled
#define	GSC_WAIT_FLAG_DONE				0x0002	// a desired event occurred
#define	GSC_WAIT_FLAG_TIMEOUT			0x0004	// the timeout period lapsed
#define	GSC_WAIT_FLAG_ALL				0x0007	// all flags
#define	GSC_WAIT_FLAG_INTERNAL			0x8000	// used internally

// Wait macros: gsc_wait_t.main
#define	GSC_WAIT_MAIN_PCI				0x0001	// main PCI interrupt
#define	GSC_WAIT_MAIN_DMA0				0x0002	// DMA0 done
#define	GSC_WAIT_MAIN_DMA1				0x0004	// DMA1 done
#define	GSC_WAIT_MAIN_GSC				0x0008	// firmware or similar interrupt
#define	GSC_WAIT_MAIN_OTHER				0x0010	// most likely a shared interrupt
#define	GSC_WAIT_MAIN_SPURIOUS			0x0020	// an unexpected interrupt
#define	GSC_WAIT_MAIN_UNKNOWN			0x0040	// device interrupt of unknown origin
#define	GSC_WAIT_MAIN_ALL				0x007F

// Wait macros: gsc_wait_t.io
#define	GSC_WAIT_IO_RX_DONE				0x0001
#define	GSC_WAIT_IO_RX_ERROR			0x0002
#define	GSC_WAIT_IO_RX_TIMEOUT			0x0004
#define	GSC_WAIT_IO_RX_ABORT			0x0008
#define	GSC_WAIT_IO_RX_ALL				0x000F
#define	GSC_WAIT_IO_TX_DONE				0x0010
#define	GSC_WAIT_IO_TX_ERROR			0x0020
#define	GSC_WAIT_IO_TX_TIMEOUT			0x0040
#define	GSC_WAIT_IO_TX_ABORT			0x0080
#define	GSC_WAIT_IO_TX_ALL				0x00F0
#define	GSC_WAIT_IO_ALL					0x00FF

// Wait macros: gsc_wait_t.timeout (in seconds)
#define	GSC_WAIT_TIMEOUT_MAX			(60L * 60L * 1000L)	// 1 hour



// data types	***************************************************************

typedef enum
{								// Vendor	Device	SubVen	SubDev	Additional
	GSC_DEV_TYPE_ADADIO,		// 0x10B5	0x9080	0x10B5	0x2370
	GSC_DEV_TYPE_6SDI,			// 0x10B5	0x9080	0x10B5	0x2408
	GSC_DEV_TYPE_16SDI,			// 0x10B5	0x9080	0x10B5	0x2371
	GSC_DEV_TYPE_16SDI_HS,		// 0x10B5	0x9080	0x10B5	0x2449
	GSC_DEV_TYPE_16HSDI,		// 0x10B5	0x9080	0x10B5	0x2306
	GSC_DEV_TYPE_24DSI12,		// 0x10B5	0x9080	0x10B5	0x3100	BrdCfg.D22=0
	GSC_DEV_TYPE_24DSI32,		// 0x10B5	0x9080	0x10B5	0x2974
	GSC_DEV_TYPE_24DSI6,		// 0x10B5	0x9080	0x10B5	0x3100	BrdCfg.D22=1
	GSC_DEV_TYPE_16AI32SSC,		// 0x10B5	0x9056	0x10B5	0x3101	@0x40 != BCR, @0x80 != BCR
	GSC_DEV_TYPE_18AI32SSC1M,	// 0x10B5	0x9056	0x10B5	0x3101	@0x40 != BCR, @0x80 == BCR
								// 0x10B5	0x9056	0x10B5	0x3431	This board has two id means.
	GSC_DEV_TYPE_16AO20,		// 0x10B5	0x9080	0x10B5	0x3102
	GSC_DEV_TYPE_16AISS16AO2,	// 0x10B5	0x9056	0x10B5	0x3243
	GSC_DEV_TYPE_16HSDI4AO4,	// 0x10B5	0x9056	0x10B5	0x3428
	GSC_DEV_TYPE_12AISS8AO4,	// 0x10B5	0x9080	0x10B5	0x2880
	GSC_DEV_TYPE_16AO16,		// 0x10B5	0x9056	0x10B5	0x3120
	GSC_DEV_TYPE_16AIO,			// 0x10B5	0x9080	0x10B5	0x2402
	GSC_DEV_TYPE_12AIO,			// 0x10B5	0x9080	0x10B5	0x2409
	GSC_DEV_TYPE_16AO12,		// 0x10B5	0x9056	0x10B5	0x3120
	GSC_DEV_TYPE_SIO4,			// 0x10B5	0x9080	0x10B5	0x2401	FW Type = 0x00 or 0x01
								// 0x10B5	0x9056	0x10B5	0x3198
	GSC_DEV_TYPE_SIO4_SYNC,		// 0x10B5	0x9080	0x10B5	0x2401	FW Type = 0x04
								// 0x10B5	0x9056	0x10B5	0x3198
	GSC_DEV_TYPE_14HSAI4,		// 0x10B5	0x9056	0x10B5	0x3300
	GSC_DEV_TYPE_HPDI32,		// 0x10B5	0x9080	0x10B5	0x2400	32-bit
								// 0x10B5	0x9656	0x10B5	0x2705	64-bit
	GSC_DEV_TYPE_OPTO16X16,		// 0x10B5	0x9056	0x10B5	0x3460
	GSC_DEV_TYPE_16AI32SSA,		// 0x10B5	0x9056	0x10B5	0x3101	@0x40 == BCR, @0x80 == BCR
	GSC_DEV_TYPE_24DSI16WRC,	// 0x10B5	0x9056	0x10B5	0x3466
	GSC_DEV_TYPE_16AIO168,		// 0x10B5	0x9080	0x10B5	0x2879
	GSC_DEV_TYPE_OPTO32,		// 0x10B5	0x906E	0x10B5	0x9080
								// 0x10B5	0x9056	0x10B5	0x3471
} gsc_dev_type_t;

typedef enum
{
	GSC_IO_MODE_PIO,		// Programmed I/O
	GSC_IO_MODE_DMA,		// Block mode DMA
	GSC_IO_MODE_DMDMA		// Demand Mode DMA
} gsc_io_mode_t;

typedef struct
{
	__u32	reg;	// range: any valid register definition
	__u32	value;	// range: 0x0-0xFFFFFFFF
	__u32	mask;	// range: 0x0-0xFFFFFFFF
} gsc_reg_t;

typedef enum
{
	GSC_VPD_TYPE_NAME,
	GSC_VPD_TYPE_MODEL_NUMBER,
	GSC_VPD_TYPE_SERIAL_NUMBER
} gsc_vpd_type_t;

typedef struct
{
	__s32	type;
	__s32	reserved;
	__u8	data[128];
} gsc_vpd_t;

typedef struct
{
	__u32	flags;		// done, timeout, cancel, ...
	__u32	main;		// interrupts: PCI, DMA0, DMA1, Local, Other
	__u32	gsc;		// firmware specific interrupts
	__u32	alt;		// additional device interrupts, Ex. SIO4 Zilog interrupts
	__u32	io;			// read and write call completion
	__u32	timeout_ms;	// milliseconds
	__u32	count;		// status: number of awaiting threads meeting any criteria
						// cancel: number of waits cancelled
} gsc_wait_t;



#endif
