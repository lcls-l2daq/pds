// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/LINUX/16AI32SSC/driver/main.h $
// $Rev$
// $Date$

#ifndef __MAIN_H__
#define __MAIN_H__

#define	DEV_BAR_SHOW		0
#define	DEV_PCI_ID_SHOW		0

#include "16ai32ssc.h"

#include "gsc_common.h"
#include "gsc_main.h"



// #defines	*******************************************************************

#define	DEV_MODEL					"16AI32SSC"	// Upper case form of the below.
#define	DEV_NAME					"16ai32ssc"	// MUST AGREE WITH AI32SSC_BASE_NAME

#define DEV_VERSION					"1.9"		// FOR DEVICE SPECIFIC CODE ONLY!
// 1.9	Added ISR support for pre-005 firmware.
//		Fixed the IRQ0 and IRQ1 Select IOCTL services for pre-005 firmware.
// 1.8	Corrected a compile bug for Fedora 7.
// 1.7	Corrected expected initialize and auto-cal periods.
//		Corrected an Input Buffer Threshold Status service bug.
// 1.6	Added the AI32SSC_IOCTL_SCAN_MARKER_VAL IOCTL service.
//		Implemented support for the common PIO I/O routines.
//		Changed use of DEV_SUPPORTS_PROC_ID_STR macro.
//		Changed use of DEV_SUPPORTS_READ macro.
//		Changed use of DEV_SUPPORTS_WRITE macro.
//		Changed use of DEV_IO_AUTO_START macro.
// 1.5	Made the Auto-Calibration IOCTL code more robust.
//		Made the Initialization IOCTL code more robust.
//		Fixed AI32SSC_IOCTL_RX_IO_ABORT service bug.
//		Fixed AI32SSC_IOCTL_TT_CHAN_MASK definition.
// 1.4	Overhauled IOCTL based local interrupt implementation.
//		Updated per changes to the common source code.
//		: Added DEV_IO_AUTO_START macro - auto-start not support.
//		: Added DEV_SUPPORTS_PROC_ID_STR macro - not supported.
//		: Implemented IOCTL based I/O Abort support.
//		: Implemented IOCTL based event/interrupt wait support.
//		: Updated to use the common gsc_open() and gsc_close() services.
//		: Implemented explicit local interrupt support.
// 1.3	Corrected a bug in the AI32SSC_IOCTL_AIN_BUF_CLEAR service.
//		Remived unused #defines.
// 1.2	Updated the read services to use common argument data types.
//		Fixed a typo/bug in dma_read_available.
//		Updated the driver to use the common open and close service calls.
//		Fixed a bug in the Scan Marker IOCTL service.
// 1.1	Added the dev_check_id() function in device.c.
// 1.0	Initial release of the new driver supporting the basic 16AI32SSC.

// I/O services
#define	DEV_PIO_READ_AVAILABLE		pio_read_available
#define	DEV_PIO_READ_WORK			gsc_read_pio_work_32_bit
#define	DEV_DMA_READ_AVAILABLE		dma_read_available
#define	DEV_DMA_READ_WORK			dma_read_work
#define	DEV_DMDMA_READ_AVAILABLE	dmdma_read_available
#define	DEV_DMDMA_READ_WORK			dmdma_read_work

#define	DEV_SUPPORTS_READ
#define	GSC_READ_PIO_WORK_32_BIT

// WAIT services
#define	DEV_WAIT_GSC_ALL			AI32SSC_WAIT_GSC_ALL
#define	DEV_WAIT_ALT_ALL			AI32SSC_WAIT_ALT_ALL



// data types	***************************************************************

struct _dev_io_t
{
	gsc_sem_t				sem;				// Only one Tx or Rx at a time.

	int						abort;
	int						bytes_per_sample;	// Sample size in bytes.
	gsc_dma_ch_t*			dma_channel;		// Use this channel for DMA.
	gsc_io_mode_t			io_mode;			// PIO, DMA, DMDMA
	u32						io_reg_offset;		// Offset of board's I/O FIFO.
	VADDR_T					io_reg_vaddr;		// Address of board's I/O FIFO.
	int						non_blocking;		// Is this non-blocking I/O?
	s32						overflow_check;		// Check overflow when reading?
	s32						pio_threshold;		// Use PIO if samples <= this.
	s32						timeout_s;			// I/O timeout in seconds.
	s32						underflow_check;	// Check underflow when reading?

	// Memory allocations.
	unsigned long			adrs;				// From get free pages.
	void*					buf;				// Page aligned.
	s32						bytes;				// Rounded to page boundary.
	u8						order;				// Order of the page request.

	// Thread sleep and resume fields.
	WAIT_QUEUE_HEAD_T		queue;
	int						condition;			// 2.6 kernel
};

struct _dev_data_t
{
	struct pci_dev*		pci;			// the kernel PCI device descriptor
	gsc_sem_t			sem;			// Comtrol access to structure
	int					in_use;			// Only one user at a time.
	int					board_index;	// Corresponds to minor number.
	const char*			model;			// i.e. "16AI32SSC"
	gsc_dev_type_t		board_type;		// Corresponds to the model string.

	gsc_bar_t			plx;			// PLX registers in memory space.
	gsc_bar_t			bar1;			// PLX registers in I/O space.
	gsc_bar_t			gsc;			// GSC registers in memory space.
	gsc_bar_t			bar3;			// May be unused.
	gsc_bar_t			bar4;			// May be unused.
	gsc_bar_t			bar5;			// May be unused.

	gsc_dma_t			dma;			// For DMA based I/O.

	gsc_irq_t			irq;			// For interrut support.

	dev_io_t			rx;				// For read operations.

	gsc_wait_node_t*	wait_list;

	struct
	{
		VADDR_T			plx_intcsr_32;	// Interrupt Control/Status Register
		VADDR_T			plx_dmaarb_32;	// DMA Arbitration Register
		VADDR_T			plx_dmathr_32;	// DMA Threshold Register

		VADDR_T			gsc_bctlr_32;	// 0x00 Board Control Register
		VADDR_T			gsc_icr_32;		// 0x04 Interrupt Control Register
		VADDR_T			gsc_ibdr_32;	// 0x08 Input Buffer Data Register
		VADDR_T			gsc_ibcr_32;	// 0x0C Input Buffer Control Register

		VADDR_T			gsc_bufsr_32;	// 0x18 Buffer Size Register

		VADDR_T			gsc_acar_32;	// 0x24 Active Channel Assignment Register
		VADDR_T			gsc_bcfgr_32;	// 0x28 Board Configuration Register

		VADDR_T			gsc_smlwr_32;	// 0x38 Scan Marker Lower Word Register
		VADDR_T			gsc_smuwr_32;	// 0x3C Scan Marker Upper Word Register

		// These are the Time Tag registers
		VADDR_T			gsc_ttcr_32;	// 0x50 Time Tag Cinfiguration Register
		VADDR_T			gsc_ttacmr_32;	// 0x54 Time Tag Active Channel Mask Register

	} vaddr;

	struct
	{
		u32				gsc_bcfgr_32;		// Board Configuration Register
		u32				fw_ver;

		s32				auto_cal_ms;		// Maximum ms for auto-cal

		s32				channels_max;		// Maximum channels supported by model.
		s32				channel_qty;		// The number of channels on the board.

		u32				fifo_size;			// Size of FIFO - not the fill level.
		s32				fsamp_max;			// The maximum Fsamp rate per channel.
		s32				fsamp_min;			// The minimum Fsamp rate per channel.

		s32				initialize_ms;		// Maximum ms for initialize

		s32				master_clock;		// Master clock frequency

		s32				rate_gen_fgen_max;	// Rate Generator maximum output rate.
		s32				rate_gen_fgen_min;	// Rate Generator minimum output rate.
		s32				rate_gen_nrate_mask;// Mask of valid Nrate bits.
		s32				rate_gen_nrate_max;	// Minimum Nrate value.
		s32				rate_gen_nrate_min;	// Maximum Nrate value.
		s32				rate_gen_qty;		// The number of rate generators on board.

		s32				time_tag_support;	// Is Time Tag support present?

	} cache;
};



// prototypes	***************************************************************

ssize_t		dma_read_available(dev_data_t* dev, ssize_t samples);
ssize_t		dma_read_work(dev_data_t* dev, char* buff, ssize_t samples, unsigned long jif_end);
ssize_t		dmdma_read_available(dev_data_t* dev, ssize_t samples);
ssize_t		dmdma_read_work(dev_data_t* dev, char* buff, ssize_t samples, unsigned long jif_end);

int			initialize_ioctl(dev_data_t* dev, void* arg);
void		io_close(dev_data_t* dev);
int			io_create(dev_data_t* dev);
void		io_destroy(dev_data_t* dev);
int			io_open(dev_data_t* dev);

ssize_t		pio_read_available(dev_data_t* dev, ssize_t samples);

void		reg_mem32_mod(VADDR_T vaddr, u32 value, u32 mask);



#endif
