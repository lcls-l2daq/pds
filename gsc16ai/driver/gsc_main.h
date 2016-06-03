// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_main.h $
// $Rev$
// $Date$

#ifndef __GSC_MAIN_H__
#define __GSC_MAIN_H__

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/page.h>
#include <asm/types.h>
#include <asm/uaccess.h>

#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/wait.h>

#include "gsc_kernel_2_2.h"
#include "gsc_kernel_2_4.h"
#include "gsc_kernel_2_6.h"
#include "gsc_common.h"

#ifndef CONFIG_PCI
	#error This driver requires PCI support.
#endif



// #defines	*******************************************************************

// This is the OVERALL version number.
#define GSC_DRIVER_VERSION			DEV_VERSION "." GSC_COMMON_VERSION

// This is for the common code only!
#define	GSC_COMMON_VERSION			"22"
// 22	Added support for the OPTO32 boards.
//		Added support for the PCI9060ES.
// 21	Removed compiler warning in Fedora 12: reduced module_init stack usage.
// 20	Added support for the 16AIO168.
//		Removed remove_proc_entry() call from proc_start - fix for Fedora 14.
//		Fixed a bug in gsc_ioctl_init().
// 19	Added support for the 24DSI16WRC.
// 18	Added common PIO read and write routines.
//		Changed use of DEV_SUPPORTS_PROC_ID_STR macro.
//		Changed use of DEV_SUPPORTS_READ macro.
//		Changed use of DEV_SUPPORTS_WRITE macro.
//		Added initial support for Vital Product Data.
//		Added support for the 16AI32SSA.
// 17	Added support for the OPTO16X16.
//		Corrected a bug: wait timeouts in jiffy units are negative.
// 16	Added support for aborting active I/O operations.
//		Added support for Auto-Start operations.
//		Added wait options for I/O cancellations.
//		Fixed bugs in the DMA code evaluating lock return status.
//		Added /proc board identifier string support.
//		Added gsc_irq_local_disable and gsc_irq_local_enable;
//		Added failure message when driver doesn't load.
//		Improved timeout handling.
//		Added gsc_time.c.
// 15	Added support for the 14HSAI4 and the HPDI32.
// 14	Added wait event, wait cancel and wait status services.
// 13	Modified the EVENT_WAIT_IRQ_TO() macro for the 2.6 kernel.
//		We now no longer initialize the condition variable in the macro.
//		The condition variable must be initialized outside the macro.
//		Added support for the 16AIO and the 12AIO.
// 12	Added more id information for the 18AI32SSC1M boards.
// 11	Added module count support.
//		Added common gsc_open() and gsc_close() calls.
// 10	Added support for the 12AISS8AO4 and the 16AO16.
// 9	Added support for the 16HSDI4AO4. Fixed bug in gsc_ioctl_init().
// 8	Added support for the 16AISS16AO2.
//		Made various read() and write() support services use same data types.
// 7	Added support for the 16AO20. Implemented write() support.
// 6	Added dev_check_id() for more detailed device identification.
// 5	Fixed DMA engine initialization code.
//		This was previously and incorrectly reported here as a version 4 mod.
// 4	Added support for the 18AI32SSC1M.
//		Modified some PLX register names for consistency.
// 3	Added support for the 16AI32SSC.
// 2	Added support for the 24DSI6.
// 1	Updated the makefile's "clean" code.
//		Added code to expand access rights to makefile.dep.
//		Added 24DSI12/32 types to gsc_common.h
// 0	initial release

#define	ARRAY_ELEMENTS(a)			(sizeof((a))/sizeof((a)[0]))

#define	MS_TO_JIFFIES(m)			(((m) + ((1000 / HZ) - 1)) / (1000 / HZ))
#define	JIFFIES_TO_MS(j)			((((j) * 1000) + (HZ / 2)) / HZ)

#define	_1K							(1024L)
#define	_5K							(_1K * 5)
#define	_30K						(_1K * 30)
#define	_32K						(_1K * 32)
#define	_64K						(_1K * 64)
#define	_220K						(_1K * 220)
#define	_256K						(_1K * 256)
#define	_1100K						(_1K * 1100)
#define	_1M							(_1K * _1K)
#define	_8M							(_1M * 8)

#define	_8MHZ						( 8000000L)
#define	_9_6MHZ						( 9600000L)
#define	_16MHZ						(16000000L)
#define	_19_2MHZ					(19200000L)
#define	_38_4MHZ					(38400000L)

// Virtual address items
#define	GSC_VADDR(d,o)				(VADDR_T) (((u8*) (d)->gsc.vaddr) + (o))
#define	PLX_VADDR(d,o)				(VADDR_T) (((u8*) (d)->plx.vaddr) + (o))

// DMA
#define	GSC_DMA_CSR_DISABLE			GSC_FIELD_ENCODE(0,0,0)
#define	GSC_DMA_CSR_ENABLE			GSC_FIELD_ENCODE(1,0,0)
#define	GSC_DMA_CSR_START			GSC_FIELD_ENCODE(1,1,1)
#define	GSC_DMA_CSR_ABORT			GSC_FIELD_ENCODE(1,2,2)
#define	GSC_DMA_CSR_CLEAR			GSC_FIELD_ENCODE(1,3,3)
#define	GSC_DMA_CSR_DONE			GSC_FIELD_ENCODE(1,4,4)

#define	GSC_DMA_CAP_DMA_READ		0x01	// DMA chan can do DMA Rx
#define	GSC_DMA_CAP_DMA_WRITE		0x02	// DMA chan can do DMA Tx
#define	GSC_DMA_CAP_DMDMA_READ		0x04	// DMA chan can do DMDMA Rx
#define	GSC_DMA_CAP_DMDMA_WRITE		0x08	// DMA chan can do DMDMA Tx
#define	GSC_DMA_SEL_STATIC			0x10	// Get the DMA chan and keep it.
#define	GSC_DMA_SEL_DYNAMIC			0x20	// Hold the DMA chan only as needed.

#define	GSC_DMA_MODE_SIZE_8_BITS			GSC_FIELD_ENCODE(0, 1, 0)
#define	GSC_DMA_MODE_SIZE_16_BITS			GSC_FIELD_ENCODE(1, 1, 0)
#define	GSC_DMA_MODE_SIZE_32_BITS			GSC_FIELD_ENCODE(2, 1, 0)
#define	GSC_DMA_MODE_INPUT_ENABLE			GSC_FIELD_ENCODE(1, 6, 6)
#define	GSC_DMA_MODE_BURSTING_LOCAL			GSC_FIELD_ENCODE(1, 8, 8)
#define	GSC_DMA_MODE_INTERRUPT_WHEN_DONE	GSC_FIELD_ENCODE(1,10,10)
#define	GSC_DMA_MODE_LOCAL_ADRESS_CONSTANT	GSC_FIELD_ENCODE(1,11,11)
#define	GSC_DMA_MODE_BLOCK_DMA				GSC_FIELD_ENCODE(0,12,12)	// Non-Demand Mode
#define	GSC_DMA_MODE_DM_DMA					GSC_FIELD_ENCODE(1,12,12)	// Demand Mode
#define	GSC_DMA_MODE_PCI_INTERRUPT_ENABLE	GSC_FIELD_ENCODE(1,17,17)

#define	GSC_DMA_DPR_END_OF_CHAIN			GSC_FIELD_ENCODE(1,1,1)
#define	GSC_DMA_DPR_TERMINAL_COUNT_IRQ		GSC_FIELD_ENCODE(1,2,2)
#define	GSC_DMA_DPR_HOST_TO_BOARD			GSC_FIELD_ENCODE(0,3,3)		// Tx operation
#define	GSC_DMA_DPR_BOARD_TO_HOST			GSC_FIELD_ENCODE(1,3,3)		// Rx operation

// PLX Interrupt Control and Status Register
#define	GSC_INTCSR_MAILBOX_INT_ENABLE		GSC_FIELD_ENCODE(1, 3, 3)
#define	GSC_INTCSR_PCI_INT_ENABLE			GSC_FIELD_ENCODE(1, 8, 8)
#define	GSC_INTCSR_PCI_DOOR_INT_ENABLE		GSC_FIELD_ENCODE(1, 9, 9)
#define	GSC_INTCSR_ABORT_INT_ENABLE			GSC_FIELD_ENCODE(1,10,10)
#define	GSC_INTCSR_LOCAL_INT_ENABLE			GSC_FIELD_ENCODE(1,11,11)
#define	GSC_INTCSR_PCI_DOOR_INT_ACTIVE		GSC_FIELD_ENCODE(1,13,13)
#define	GSC_INTCSR_ABORT_INT_ACTIVE			GSC_FIELD_ENCODE(1,14,14)
#define	GSC_INTCSR_LOCAL_INT_ACTIVE			GSC_FIELD_ENCODE(1,15,15)
#define	GSC_INTCSR_LOC_DOOR_INT_ENABLE		GSC_FIELD_ENCODE(1,17,17)
#define	GSC_INTCSR_DMA_0_INT_ENABLE			GSC_FIELD_ENCODE(1,18,18)
#define	GSC_INTCSR_DMA_1_INT_ENABLE			GSC_FIELD_ENCODE(1,19,19)
#define	GSC_INTCSR_LOC_DOOR_INT_ACTIVE		GSC_FIELD_ENCODE(1,20,20)
#define	GSC_INTCSR_DMA_0_INT_ACTIVE			GSC_FIELD_ENCODE(1,21,21)
#define	GSC_INTCSR_DMA_1_INT_ACTIVE			GSC_FIELD_ENCODE(1,22,22)
#define	GSC_INTCSR_BIST_INT_ACTIVE			GSC_FIELD_ENCODE(1,23,23)
#define	GSC_INTCSR_MAILBOX_INT_ACTIVE		GSC_FIELD_ENCODE(1,28,31)

// 32-bit compatibility support.
#define	GSC_IOCTL_32BIT_ERROR				(-1)	// There is a problem.
#define	GSC_IOCTL_32BIT_NONE				0		// Support not in the kernel.
#define	GSC_IOCTL_32BIT_NATIVE				1		// 32-bit support on 32-bit OS
#define	GSC_IOCTL_32BIT_TRANSLATE			2		// globally translated "cmd"s
#define	GSC_IOCTL_32BIT_COMPAT				3		// compat_ioctl service
#define	GSC_IOCTL_32BIT_DISABLED			4		// Support is disabled.

// Data size limits.
#define	S32_MAX								(+2147483647L)



// data types	***************************************************************

typedef struct _dev_data_t	dev_data_t;	// A device specific structure.
typedef struct _dev_io_t	dev_io_t;	// A device specific structure.

typedef	int					(*gsc_ioctl_service_t)(dev_data_t* dev, void* arg);
typedef	struct semaphore	gsc_sem_t;

typedef struct
{
	int						index;		// BARx
	int						offset;		// Offset of BARx register in PCI space.
	u32						reg;		// Actual BARx register value.
	u32						flags;		// lower register bits
	int						io_mapped;	// Is this an I/O mapped region?
	unsigned long			phys_adrs;	// Physical address of region.
	u32						size;		// Region size in bytes.
	u32						requested;	// Is resource requested from OS?
	void*					vaddr;		// Kernel virtual address.
} gsc_bar_t;

typedef struct
{
	const char*				model;	// NULL model terminates a list of entries.
	u16						vendor;
	u16						device;
	u16						sub_vendor;
	u16						sub_device;
	gsc_dev_type_t			type;
} gsc_dev_id_t;

typedef struct
{
	int						index;
	int						in_use;
	unsigned int			flags;
	int						intcsr_enable;

	struct
	{
		VADDR_T				mode_32;	// DMAMODEx
		VADDR_T				padr_32;	// DMAPADRx
		VADDR_T				ladr_32;	// DMALADRx
		VADDR_T				siz_32;		// DMASIZx
		VADDR_T				dpr_32;		// DMADPRx
		VADDR_T				csr_8;		// DMACSRx
	} vaddr;

} gsc_dma_ch_t;

typedef struct
{
	gsc_sem_t				sem;		// control access
	gsc_dma_ch_t			channel[2];	// Settings and such
} gsc_dma_t;

typedef struct
{
	gsc_sem_t				sem;		// Control access.
} gsc_irq_t;

typedef struct
{
	char					built[32];
	dev_data_t*				dev_list[10];
	int						dev_qty;
	int						driver_loaded;
	struct file_operations	fops;
	int						ioctl_32bit;	// IOCTL_32BIT_XXX
	int						major_number;
	int						proc_enabled;
} gsc_global_t;

typedef struct
{
	unsigned long			cmd;	// -1 AND
	gsc_ioctl_service_t		func;	// NULL terminate the list
} gsc_ioctl_t;

typedef struct
{
	u8			loaded;			// Have we tried to load the image?
	u8			eeprom[1025];	// eeprom image is loaded here.
								// Not a multiple of 4 bytes!
	const u8*	head;			// Point to first byte of VPD, if present.
								// If the pointer is NULL when "loaded" is
								// set, then there has been an error.
} gsc_vpd_data_t;

typedef struct _gsc_wait_node_t
{
	gsc_wait_t*					wait;
	WAIT_QUEUE_ENTRY_T			entry;
	WAIT_QUEUE_HEAD_T			queue;
	int							condition;
	struct timeval				tv_start;
	struct _gsc_wait_node_t*	next;
} gsc_wait_node_t;



// variables	***************************************************************

extern	gsc_global_t		gsc_global;



// prototypes	***************************************************************

int			gsc_bar_create(struct pci_dev* pci, int index, gsc_bar_t* bar, int mem, int io);
void		gsc_bar_destroy(gsc_bar_t* bar);

int			gsc_close(struct inode *, struct file *);

dev_data_t*	gsc_dev_data_t_locate(struct inode* inode);

int			gsc_ioctl(struct inode* inode, struct file* fp, unsigned int cmd, unsigned long arg);
long		gsc_ioctl_compat(struct file* fp, unsigned int cmd, unsigned long arg);
long		gsc_ioctl_unlocked(struct file* fp, unsigned int cmd, unsigned long arg);

int			gsc_dma_abort_active_xfer(dev_data_t* dev, dev_io_t* io);
void		gsc_dma_close(dev_data_t* dev);
int			gsc_dma_create(dev_data_t* dev, u32 ch0_flags, u32 ch1_flags);
void		gsc_dma_destroy(dev_data_t* dev);
int			gsc_dma_open(dev_data_t* dev);
ssize_t		gsc_dma_perform(dev_data_t*			dev,
							dev_io_t*			io,
							unsigned long		jif_end,
							unsigned int		ability,
							u32					mode,
							u32					dpr,
							void*				buff,
							ssize_t				samples);

int			gsc_io_create(dev_data_t* dev, dev_io_t* gsc, size_t size);
void		gsc_io_destroy(dev_data_t* dev, dev_io_t* gsc);
int			gsc_ioctl_init(void);
void		gsc_ioctl_reset(void);
int			gsc_irq_access_lock(dev_data_t* dev, int isr);
void		gsc_irq_access_unlock(dev_data_t* dev, int isr);
void		gsc_irq_close(dev_data_t* dev);
int			gsc_irq_create(dev_data_t* dev);
void		gsc_irq_destroy(dev_data_t* dev);
void		gsc_irq_intcsr_mod(dev_data_t* dev, u32 value, u32 mask);
int			gsc_irq_isr_common(int irq, void* dev_id);
int			gsc_irq_local_disable(dev_data_t* dev);
int			gsc_irq_local_enable(dev_data_t* dev);
int			gsc_irq_open(dev_data_t* dev);

void		gsc_module_count_dec(void);
int			gsc_module_count_inc(void);

int			gsc_open(struct inode *, struct file *);

int			gsc_proc_read(char* page, char** start, off_t offset, int count, int* eof, void* data);
int			gsc_proc_start(void);
void		gsc_proc_stop(void);

ssize_t		gsc_read(struct file* filp, char* buf, size_t count, loff_t* offp);
int			gsc_read_abort_active_xfer(dev_data_t* dev);

// Must define GSC_READ_PIO_WORK and/or GSC_READ_PIO_WORK_XX_BIT
ssize_t		gsc_read_pio_work			(dev_data_t* dev, char* buff, ssize_t count, unsigned long jif_end);
ssize_t		gsc_read_pio_work_8_bit		(dev_data_t* dev, char* buff, ssize_t count, unsigned long jif_end);
ssize_t		gsc_read_pio_work_16_bit	(dev_data_t* dev, char* buff, ssize_t count, unsigned long jif_end);
ssize_t		gsc_read_pio_work_32_bit	(dev_data_t* dev, char* buff, ssize_t count, unsigned long jif_end);


int			gsc_reg_mod_ioctl(dev_data_t* dev, gsc_reg_t* arg);
int			gsc_reg_read_ioctl(dev_data_t* dev, gsc_reg_t* arg);
int			gsc_reg_write_ioctl(dev_data_t* dev, gsc_reg_t* arg);

void		gsc_sem_create(gsc_sem_t* sem);
void		gsc_sem_destroy(gsc_sem_t* sem);
int			gsc_sem_lock(gsc_sem_t* sem);
void		gsc_sem_unlock(gsc_sem_t* sem);

long		gsc_time_delta(unsigned long t1, unsigned long t2);

int			gsc_vpd_read_ioctl(dev_data_t* dev, gsc_vpd_t* vpd);

void		gsc_wait_close(dev_data_t* dev);
int			gsc_wait_event(	dev_data_t*		dev,
							gsc_wait_t*		wait,
							int				(*setup)(dev_data_t* dev, unsigned long arg),
							unsigned long	arg,
							gsc_sem_t*		sem);
int			gsc_wait_resume_io(dev_data_t* dev, u32 io);
int			gsc_wait_resume_irq_alt(dev_data_t* dev, u32 alt);
int			gsc_wait_resume_irq_gsc(dev_data_t* dev, u32 gsc);
int			gsc_wait_resume_irq_main(dev_data_t* dev, u32 main);
int			gsc_write_abort_active_xfer(dev_data_t* dev);
ssize_t		gsc_write(struct file *filp, const char *buf, size_t count, loff_t * offp);

// Must define GSC_WRITE_PIO_WORK and/or GSC_WRITE_PIO_WORK_XX_BIT
ssize_t		gsc_write_pio_work			(dev_data_t* dev, const char* buff, ssize_t count, unsigned long jif_end);
ssize_t		gsc_write_pio_work_8_bit	(dev_data_t* dev, const char* buff, ssize_t count, unsigned long jif_end);
ssize_t		gsc_write_pio_work_16_bit	(dev_data_t* dev, const char* buff, ssize_t count, unsigned long jif_end);
ssize_t		gsc_write_pio_work_32_bit	(dev_data_t* dev, const char* buff, ssize_t count, unsigned long jif_end);



// ****************************************************************************
// THESE ARE PROVIDED BY THE DEVICE SPECIFIC CODE

//#define	DEV_MODEL					"XXX"
//#define	DEV_NAME					"xxx"
//#define	DEV_VERSION					"x.x"
//#define	DEV_BAR_SHOW				0 or 1 (1 to show BAR info during init)
//#define	DEV_PCI_ID_SHOW				0 or 1 (1 to show ID info during init)
//#define	DEV_SUPPORTS_READ			define if read() is supported.
//#define	DEV_SUPPORTS_WRITE			define if write() is supported.
//#define	DEV_SUPPORTS_PROC_ID_STR	define if this string is supported.
//#define	DEV_SUPPORTS_VPD			Is Vital Product Data supported?
//#define	DEV_SUPPORTS_VPD_PCI9056	VPD follows PCI9056 implementation.

// If read() is supported, then the following must be provided.
//#define	DEV_PIO_READ_AVAILABLE		xxx
//#define	DEV_PIO_READ_WORK			xxx
//#define	DEV_DMA_READ_AVAILABLE		xxx
//#define	DEV_DMA_READ_WORK			xxx
//#define	DEV_DMDMA_READ_AVAILABLE	xxx
//#define	DEV_DMDMA_READ_WORK			xxx

// If write() is supported, then the following must be provided.
//#define	DEV_PIO_WRITE_AVAILABLE		xxx
//#define	DEV_PIO_WRITE_WORK			xxx
//#define	DEV_DMA_WRITE_AVAILABLE		xxx
//#define	DEV_DMA_WRITE_WORK			xxx
//#define	DEV_DMDMA_WRITE_AVAILABLE	xxx
//#define	DEV_DMDMA_WRITE_WORK		xxx

// Variables
extern	const gsc_dev_id_t	dev_id_list[];
extern	const gsc_ioctl_t	dev_ioctl_list[];

// Functions
int		dev_check_id				(dev_data_t* dev);
int		dev_close					(dev_data_t* dev);
int		dev_device_create			(dev_data_t* ref);
void	dev_device_destroy			(dev_data_t* dev);
void	dev_irq_isr_local_handler	(dev_data_t* dev);
int		dev_open					(dev_data_t* dev);
ssize_t	dev_read_startup			(dev_data_t* dev);
int		dev_reg_mod_alt				(dev_data_t* dev, gsc_reg_t* arg);
int		dev_reg_read_alt			(dev_data_t* dev, gsc_reg_t* arg);
int		dev_reg_write_alt			(dev_data_t* dev, gsc_reg_t* arg);
int		gsc_wait_cancel_ioctl		(dev_data_t* dev, void* arg);
int		gsc_wait_event_ioctl		(dev_data_t* dev, void* arg);
int		gsc_wait_status_ioctl		(dev_data_t* dev, void* arg);
int		dev_write_startup			(dev_data_t* dev);



#endif
