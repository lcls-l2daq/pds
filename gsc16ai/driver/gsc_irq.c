// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_irq.c $
// $Rev: 5777 $
// $Date$

#include "gsc_main.h"
#include "main.h"



/******************************************************************************
*
*	Function:	gsc_irq_access_lock
*
*	Purpose:
*
*		Apply a locking mechanism to prevent simultaneous access to the
*		device's IRQ substructure.
*
*	Arguments:
*
*		dev		The data for the device of interest.
*
*		isr		Is this an ISR based request?
*
*	Returned:
*
*		0		The request was satisfied.
*		else	There was a problem.
*
******************************************************************************/

int gsc_irq_access_lock(dev_data_t* dev, int isr)
{
	int	status;

	if (isr)
	{
		status	= 0;
#ifdef __SMP__
#else
#endif
	}
	else
	{
		status	= gsc_sem_lock(&dev->irq.sem);

		if (status == 0)
			disable_irq(dev->pci->irq);
	}

	return(status);
}



/******************************************************************************
*
*	Function:	gsc_irq_access_unlock
*
*	Purpose:
*
*		Remove the locking mechanism that prevented simultaneous access to the
*		device's IRQ substructure.
*
*	Arguments:
*
*		dev		The data for the device of interest.
*
*		isr		Is this an ISR based request?
*
*	Returned:
*
*		None.
*
******************************************************************************/

void gsc_irq_access_unlock(dev_data_t* dev, int isr)
{
	if (isr)
	{
#ifdef __SMP__
#else
#endif
	}
	else
	{
		enable_irq(dev->pci->irq);
		gsc_sem_unlock(&dev->irq.sem);
	}
}



/******************************************************************************
*
*	Function:	gsc_irq_close
*
*	Purpose:
*
*		Perform IRQ actions appropriate for closing a device.
*
*	Arguments:
*
*		dev	The device of interest.
*
*	Returned:
*
*		None.
*
******************************************************************************/

void gsc_irq_close(dev_data_t* dev)
{
	writel(0, dev->vaddr.plx_intcsr_32);
	free_irq(dev->pci->irq, dev);
}



/******************************************************************************
*
*	Function:	gsc_irq_create
*
*	Purpose:
*
*		Perform a one time initialization of the structure.
*
*	Arguments:
*
*		dev		The device of interest.
*
*	Returned:
*
*		0		All went well.
*		< 0		The code for the error encounterred.
*
******************************************************************************/

int gsc_irq_create(dev_data_t* dev)
{
	gsc_sem_create(&dev->irq.sem);
	writel(0, dev->vaddr.plx_intcsr_32);
	return(0);
}



/******************************************************************************
*
*	Function:	gsc_irq_destroy
*
*	Purpose:
*
*		Perform a one time tear down of the structure.
*
*	Arguments:
*
*		dev	The device of interest.
*
*	Returned:
*
*		None.
*
******************************************************************************/

void gsc_irq_destroy(dev_data_t* dev)
{
	if (dev->vaddr.plx_intcsr_32)
		writel(0, dev->vaddr.plx_intcsr_32);
}



/******************************************************************************
*
*	Function:	gsc_irq_intcsr_mod
*
*	Purpose:
*
*		Modify the PLX INTCSR register with restricted access. THIS IS FOR NON
*		ISR_ACCESSES ONLY!
*
*	Arguments:
*
*		dev		The data for the device of interest.
*
*		value	This is the value to apply.
*
*		mask	This is the set of bits to modify.
*
*	Returned:
*
*		None.
*
******************************************************************************/

void gsc_irq_intcsr_mod(dev_data_t* dev, u32 value, u32 mask)
{
	u32	intcsr;
	int	status;

	status	= gsc_irq_access_lock(dev, 0);
	intcsr	= readl(dev->vaddr.plx_intcsr_32);
	intcsr	= (intcsr & ~mask) | (value & mask);
	writel(intcsr, dev->vaddr.plx_intcsr_32);

	if (status == 0)
		gsc_irq_access_unlock(dev, 0);
}



/******************************************************************************
*
*	Function:	gsc_irq_open
*
*	Purpose:
*
*		Perform IRQ actions appropriate for closing a device.
*
*	Arguments:
*
*		dev	The device of interest.
*
*	Returned:
*
*		0	All went well.
*		<0	An error code for the problem seen.
*
******************************************************************************/

int gsc_irq_open(dev_data_t* dev)
{
	int	i;
	int	ret;
	u32	val	= GSC_INTCSR_PCI_INT_ENABLE | GSC_INTCSR_LOCAL_INT_ENABLE;

	i	= IRQ_REQUEST(dev, gsc_irq_isr);
	ret	= i ? -EBUSY : 0;

	writel(val, dev->vaddr.plx_intcsr_32);
	return(ret);
}



/******************************************************************************
*
*	Function:	gsc_irq_isr_common
*
*	Purpose:
*
*		Service an interrupt. For safety sake, each detected interrupt is
*		cleared then disabled.
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
*		0		The interrupt was NOT ours.
*		1		The interrupt was ours.
*
******************************************************************************/

int gsc_irq_isr_common(int irq, void* dev_id)
{
	dev_data_t*		dev		= dev_id;
	gsc_dma_ch_t*	dma;
	int				handled;
	u32				intcsr;	// PLX Interrupt Control/Status Register

	gsc_irq_access_lock(dev, 1);

	for (;;)
	{
		intcsr	= readl(dev->vaddr.plx_intcsr_32);

		// PCI ****************************************************************

		if ((intcsr & GSC_INTCSR_PCI_INT_ENABLE) == 0)
		{
			// We don't have interrupts enabled. This isn't ours.
			handled	= 0;
			gsc_wait_resume_irq_main(dev, GSC_WAIT_MAIN_PCI | GSC_WAIT_MAIN_OTHER);
			break;
		}

		// DMA 0 **************************************************************

		if ((intcsr & GSC_INTCSR_DMA_0_INT_ENABLE) &&
			(intcsr & GSC_INTCSR_DMA_0_INT_ACTIVE))
		{
			// This is a DMA0 interrupt.
			handled	= 1;
			dma		= &dev->dma.channel[0];

			// Clear the DMA DONE interrupt.
			writeb(GSC_DMA_CSR_CLEAR, dma->vaddr.csr_8);

			// Disable the DMA 0 interrupt.
			intcsr	&= ~GSC_INTCSR_DMA_0_INT_ENABLE;
			writel(intcsr, dev->vaddr.plx_intcsr_32);

			// Resume any blocked threads.
			gsc_wait_resume_irq_main(dev, GSC_WAIT_MAIN_PCI | GSC_WAIT_MAIN_DMA0);
			break;
		}

		// DMA 1 **************************************************************

		if ((intcsr & GSC_INTCSR_DMA_1_INT_ENABLE) &&
			(intcsr & GSC_INTCSR_DMA_1_INT_ACTIVE))
		{
			// This is a DMA1 interrupt.
			handled	= 1;
			dma		= &dev->dma.channel[1];

			// Clear the DMA DONE interrupt.
			writeb(GSC_DMA_CSR_CLEAR, dma->vaddr.csr_8);

			// Disable the DMA 1 interrupt.
			intcsr	&= ~GSC_INTCSR_DMA_1_INT_ENABLE;
			writel(intcsr, dev->vaddr.plx_intcsr_32);

			// Resume any blocked threads.
			gsc_wait_resume_irq_main(dev, GSC_WAIT_MAIN_PCI | GSC_WAIT_MAIN_DMA1);
			break;
		}

		// LOCAL **************************************************************

		if ((intcsr & GSC_INTCSR_LOCAL_INT_ENABLE) &&
			(intcsr & GSC_INTCSR_LOCAL_INT_ACTIVE))
		{
			// This is a LOCAL interrupt.
			handled	= 1;

			// Resume any blocked threads.
			gsc_wait_resume_irq_main(dev, GSC_WAIT_MAIN_PCI | GSC_WAIT_MAIN_GSC);

			// Let device specific code process local interrupts.
			dev_irq_isr_local_handler(dev);
			break;
		}

		// MAIL BOX ***********************************************************

		if ((intcsr & GSC_INTCSR_MAILBOX_INT_ENABLE) &&
			(intcsr & GSC_INTCSR_MAILBOX_INT_ACTIVE))
		{
			// This is an unexpected MAIL BOX interrupt.
			handled	= 1;

			// Disable the MAIL BOX interrupt.
			intcsr	&= ~GSC_INTCSR_MAILBOX_INT_ENABLE;
			writel(intcsr, dev->vaddr.plx_intcsr_32);

			// Resume any blocked threads.
			gsc_wait_resume_irq_main(dev, GSC_WAIT_MAIN_PCI | GSC_WAIT_MAIN_SPURIOUS);
			break;
		}

		// PCI DOORBELL *******************************************************

		if ((intcsr & GSC_INTCSR_PCI_DOOR_INT_ENABLE) &&
			(intcsr & GSC_INTCSR_PCI_DOOR_INT_ACTIVE))
		{
			// This is an unexpected PCI DOORBELL interrupt.
			handled	= 1;

			// Disable the PCI DOORBELL interrupt.
			intcsr	&= ~GSC_INTCSR_PCI_DOOR_INT_ENABLE;
			writel(intcsr, dev->vaddr.plx_intcsr_32);

			// Resume any blocked threads.
			gsc_wait_resume_irq_main(dev, GSC_WAIT_MAIN_PCI | GSC_WAIT_MAIN_SPURIOUS);
			break;
		}

		// ABORT **************************************************************

		if ((intcsr & GSC_INTCSR_ABORT_INT_ENABLE) &&
			(intcsr & GSC_INTCSR_ABORT_INT_ACTIVE))
		{
			// This is an unexpected ABORT interrupt.
			handled	= 1;

			// Disable the ABORT interrupt.
			intcsr	&= ~GSC_INTCSR_ABORT_INT_ENABLE;
			writel(intcsr, dev->vaddr.plx_intcsr_32);

			// Resume any blocked threads.
			gsc_wait_resume_irq_main(dev, GSC_WAIT_MAIN_PCI | GSC_WAIT_MAIN_SPURIOUS);
			break;
		}

		// LOCAL DOORBELL *****************************************************

		if ((intcsr & GSC_INTCSR_LOC_DOOR_INT_ENABLE) &&
			(intcsr & GSC_INTCSR_LOC_DOOR_INT_ACTIVE))
		{
			// This is an unexpected Local DOORBELL interrupt.
			handled	= 1;

			// Disable the Local DOORBELL interrupt.
			intcsr	&= ~GSC_INTCSR_LOC_DOOR_INT_ENABLE;
			writel(intcsr, dev->vaddr.plx_intcsr_32);

			// Resume any blocked threads.
			gsc_wait_resume_irq_main(dev, GSC_WAIT_MAIN_PCI | GSC_WAIT_MAIN_SPURIOUS);
			break;
		}

		// BIST ***************************************************************

		if (intcsr & GSC_INTCSR_BIST_INT_ACTIVE)
		{
			// This is an unexpected BIST interrupt.
			handled	= 1;

			// Service the interrupt.
			pci_write_config_byte(dev->pci, 0x0F, 0);

			// Resume any blocked threads.
			gsc_wait_resume_irq_main(dev, GSC_WAIT_MAIN_PCI | GSC_WAIT_MAIN_SPURIOUS);
			break;
		}

		// This is not one of our interrupts.
		handled	= 0;

		// Resume any blocked threads.
		gsc_wait_resume_irq_main(dev, GSC_WAIT_MAIN_PCI | GSC_WAIT_MAIN_OTHER);
		break;
	}

	gsc_irq_access_unlock(dev, 1);
	return(handled);
}



/******************************************************************************
*
*	Function:	gsc_irq_local_disable
*
*	Purpose:
*
*		Disable local interrupts. This is for non-ISR use only.
*
*	Arguments:
*
*		dev		The data for the device of interest.
*
*	Returned:
*
*		0		The request was satisfied.
*		else	There was a problem.
*
******************************************************************************/

int gsc_irq_local_disable(dev_data_t* dev)
{
	u32	intcsr;
	u32	mask	= GSC_INTCSR_LOCAL_INT_ENABLE;
	int	status;

	status	= gsc_irq_access_lock(dev, 0);
	intcsr	= readl(dev->vaddr.plx_intcsr_32);
	intcsr	= intcsr & ~mask;
	writel(intcsr, dev->vaddr.plx_intcsr_32);

	if (status == 0)
		gsc_irq_access_unlock(dev, 0);

	return(status);
}



/******************************************************************************
*
*	Function:	gsc_irq_local_enable
*
*	Purpose:
*
*		Enable local interrupts. This is for non-ISR use only.
*
*	Arguments:
*
*		dev		The data for the device of interest.
*
*	Returned:
*
*		0		The request was satisfied.
*		else	There was a problem.
*
******************************************************************************/

int gsc_irq_local_enable(dev_data_t* dev)
{
	u32	intcsr;
	u32	mask	= GSC_INTCSR_LOCAL_INT_ENABLE;
	int	status;

	status	= gsc_irq_access_lock(dev, 0);
	intcsr	= readl(dev->vaddr.plx_intcsr_32);
	intcsr	= intcsr | mask;
	writel(intcsr, dev->vaddr.plx_intcsr_32);

	if (status == 0)
		gsc_irq_access_unlock(dev, 0);

	return(status);
}


