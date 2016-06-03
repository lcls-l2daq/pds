// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_kernel_2_6.c $
// $Rev$
// $Date$

//	Provide special handling for things specific to the 2.6 kernel.

#include <linux/module.h>
#include <linux/version.h>



#if	(LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)) && \
	(LINUX_VERSION_CODE <  KERNEL_VERSION(2,7,0))

#include "gsc_main.h"
#include "main.h"



// Tags	***********************************************************************

MODULE_AUTHOR("General Standards Corporation, www.generalstandards.com");
MODULE_DESCRIPTION(DEV_MODEL " driver " GSC_DRIVER_VERSION);
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE(DEV_MODEL);



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
*		regs	Unused. Not present beginning at 2.6.19.
*
*	Returned:
*
*		The status as to whether we handled the interrupt or not.
*
******************************************************************************/

irqreturn_t gsc_irq_isr(int irq, void* dev_id  ISR_ARG3)
{
	int			handled;
	irqreturn_t	ret;

	handled	= gsc_irq_isr_common(irq, dev_id);
	ret		= handled ? IRQ_HANDLED : IRQ_NONE;
	return(ret);
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
	module_put(THIS_MODULE);
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
	int	ret;

	ret	= try_module_get(THIS_MODULE);
	ret	= ret ? 0 : -ENODEV;
	return(ret);
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
*	Returned:
*
*		int		The number of characters written.
*
******************************************************************************/

#if PROC_GET_INFO_SUPPORTED
int gsc_proc_get_info(
	char*	page,
	char**	start,
	off_t	offset,
	int		count)
{
	int	eof;
	int	i;

	i	= gsc_proc_read(page, start, offset, count, &eof, NULL);
	return(i);
}
#endif



#endif
