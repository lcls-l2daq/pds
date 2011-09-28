// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_kernel_2_2.c $
// $Rev: 3890 $
// $Date$

//	Provide special handling for things specific to the 2.2 kernel.

#include <linux/version.h>



#if	(LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,0)) && \
	(LINUX_VERSION_CODE <  KERNEL_VERSION(2,3,0))

#define	MODULE
#include "gsc_main.h"
#include "main.h"


// Tags	***********************************************************************

EXPORT_NO_SYMBOLS;
MODULE_AUTHOR("General Standards Corporation, www.generalstandards.com");
MODULE_DESCRIPTION(DEV_MODEL " driver " GSC_DRIVER_VERSION);
MODULE_SUPPORTED_DEVICE(DEV_MODEL);



/******************************************************************************
*
*	Function:	gsc_bar_size
*
*	Purpose:
*
*		Get the size of a BAR region.
*
*	Arguments:
*
*		pci		The PCI structure for our device.
*
*		bar		The index of the BAR region.
*
*	Returned:
*
*		The size of the region in bytes.
*
******************************************************************************/

u32 gsc_bar_size(struct pci_dev* pci, u8 bar)
{
	int	offset	= 0x10 + (4 * bar);
	u32	reg;
	u32	size;

	pci_read_config_dword(pci, offset, &reg);
	// Use a PCI feature to get the address mask.
	pci_write_config_dword(pci, offset, 0xFFFFFFFF);
	pci_read_config_dword(pci, offset, &size);
	// Restore the register.
	pci_write_config_dword(pci, offset, reg);
	size	= 1 + ~(size & 0xFFFFFFF0);

	return(size);
}



/******************************************************************************
*
*	Function:	gsc_module_count_dec
*
*	Purpose:
*
*		Decrement the module usage count, as appropriate for the kernel.
*
*	Arguments:
*
*		None.
*
*	Returned:
*
*		None.
*
******************************************************************************/

void gsc_module_count_dec(void)
{
	MOD_DEC_USE_COUNT;
}



/******************************************************************************
*
*	Function:	gsc_module_count_inc
*
*	Purpose:
*
*		Increment the module usage count, as appropriate for the kernel.
*
*	Arguments:
*
*		None.
*
*	Returned:
*
*		0		All went well.
*		< 0		An approoriate error code.
*
******************************************************************************/

int gsc_module_count_inc(void)
{
	MOD_INC_USE_COUNT;
	return(0);
}



/******************************************************************************
*
*	Function:	gsc_reg_pci_read_u16
*
*	Purpose:
*
*		Read a PCI configuration register.
*
*	Arguments:
*
*		pci		The PCI structure for our device.
*
*		offset	The offset of the register we're to read.
*
*	Returned:
*
*		The value read from the register.
*
******************************************************************************/

u16 gsc_reg_pci_read_u16(struct pci_dev* pci, u8 offset)
{
	u16	val;

	pci_read_config_word(pci, offset, &val);
	return(val);
}



/******************************************************************************
*
*	Function:	gsc_irq_isr
*
*	Purpose:
*
*		Service an interrupt.
*
*	Arguments:
*
*		irq		The interrupt number.
*
*		dev_id	The private data we've associated with the IRQ.
*
*		regs	Unused.
*
*	Returned:
*
*		None.
*
******************************************************************************/

void gsc_irq_isr(int irq, void* dev_id, struct pt_regs* regs)
{
	gsc_irq_isr_common(irq, dev_id);
}



/******************************************************************************
*
*	Function:	gsc_proc_get_info
*
*	Purpose:
*
*		Implement the get_info() service for /proc file system support.
*
*	Arguments:
*
*		page	The data produced is put here.
*
*		start	Records pointer to where the data in "page" begins.
*
*		offset	The offset into the file where we're to begin.
*
*		count	The limit to the amount of data we can produce.
*
*		dummy	A parameter that is unused.
*
*	Returned:
*
*		int		The number of characters written.
*
******************************************************************************/

int gsc_proc_get_info(
	char*	page,
	char**	start,
	off_t	offset,
	int		count,
	int		dummy)
{
	int	eof;
	int	i;

	i	= gsc_proc_read(page, start, offset, count, &eof, NULL);
	return(i);
}



#endif
