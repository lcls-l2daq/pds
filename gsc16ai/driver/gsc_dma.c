// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_dma.c $
// $Rev$
// $Date$

#include "gsc_main.h"
#include "main.h"



// #defines *******************************************************************

#ifndef DEV_IO_AUTO_START
	#define	DEV_IO_AUTO_START(d,i)
#endif



//*****************************************************************************
static int _dma_start(dev_data_t* dev, unsigned long arg)
{
	gsc_dma_ch_t*	dma;
	dev_io_t*		io;
	unsigned long	ul;

	io	= (void*) arg;
	dma	= io->dma_channel;

	// Start the DMA transfer.
	writeb(0, dma->vaddr.csr_8);			// Disabe
	ul	= GSC_DMA_CSR_ENABLE;
	writeb(ul, dma->vaddr.csr_8);			// Enable
	ul	= GSC_DMA_CSR_ENABLE | GSC_DMA_CSR_START;
	writeb(ul, dma->vaddr.csr_8);			// Start

	// Initiate any Auto Start activity.
	DEV_IO_AUTO_START(dev, io);

	return(0);
}



/******************************************************************************
*
*	Function:	_dma_channel_get
*
*	Purpose:
*
*		Get a PLX DMA channel for the specified operation.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		io		The I/O structure for the operation to perform.
*
*		ability	The required read/write ability.
*
*	Returned:
*
*		NULL	We couldn't get the channel.
*		else	This is the pointer to the DMA channel to use.
*
******************************************************************************/

static gsc_dma_ch_t* _dma_channel_get(
	dev_data_t*	dev,
	dev_io_t*	io,
	u32			ability)
{
	gsc_dma_ch_t*	dma;
	int				i;

	for (;;)	// Use a loop for convenience.
	{
		if (io->dma_channel)
		{
			dma	= io->dma_channel;
			break;
		}

		i	= gsc_sem_lock(&dev->dma.sem);

		if (i)
		{
			dma	= NULL;
			break;
		}

		if ((dev->dma.channel[0].in_use == 0) &&
			(dev->dma.channel[0].flags & ability))
		{
			dma			= &dev->dma.channel[0];
			dma->in_use	= 1;
			gsc_sem_unlock(&dev->dma.sem);
			break;
		}

		if ((dev->dma.channel[1].in_use == 0) &&
			(dev->dma.channel[1].flags & ability))
		{
			dma			= &dev->dma.channel[1];
			dma->in_use	= 1;
			gsc_sem_unlock(&dev->dma.sem);
			break;
		}

		dma	= NULL;
		gsc_sem_unlock(&dev->dma.sem);
		break;
	}

	return(dma);
}



/******************************************************************************
*
*	Function:	_dma_channel_reset
*
*	Purpose:
*
*		Reset the given DMA channel.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		dma		The DMA channel to reset.
*
*	Returned:
*
*		None.
*
******************************************************************************/

static void _dma_channel_reset(dev_data_t* dev, gsc_dma_ch_t* dma)
{
	int	i;

	i			= gsc_sem_lock(&dev->dma.sem);
	dma->in_use	= 0;

	if (dma->vaddr.csr_8)
	{
		writeb(GSC_DMA_CSR_DISABLE, dma->vaddr.csr_8);
		writeb(GSC_DMA_CSR_CLEAR, dma->vaddr.csr_8);
	}

	if (i == 0)
		gsc_sem_unlock(&dev->dma.sem);
}



/******************************************************************************
*
*	Function:	gsc_dma_close
*
*	Purpose:
*
*		Cleanup the DMA code for the device due to the device being closed.
*
*	Arguments:
*
*		dev		The data for the device of interest.
*
*	Returned:
*
*		None.
*
******************************************************************************/

void gsc_dma_close(dev_data_t* dev)
{
	_dma_channel_reset(dev, &dev->dma.channel[0]);
	_dma_channel_reset(dev, &dev->dma.channel[1]);
}



/******************************************************************************
*
*	Function:	gsc_dma_create
*
*	Purpose:
*
*		Perform a one time initialization of the DMA portion of the given
*		structure.
*
*	Arguments:
*
*		dev			The data for the device of interest.
*
*		ch0_flags	The DMA Channel 0 capability flags.
*
*		ch1_flags	The DMA Channel 1 capability flags.
*
*	Returned:
*
*		>= 0		The number of errors seen.
*
******************************************************************************/

int gsc_dma_create(dev_data_t* dev, u32 ch0_flags, u32 ch1_flags)
{
	gsc_dma_ch_t*	dma;

	gsc_sem_create(&dev->dma.sem);

	dma	= &dev->dma.channel[0];
	dma->index			= 0;
	dma->flags			= ch0_flags;
	dma->intcsr_enable	= GSC_INTCSR_DMA_0_INT_ENABLE;
	dma->vaddr.mode_32	= PLX_VADDR(dev, 0x80);
	dma->vaddr.padr_32	= PLX_VADDR(dev, 0x84);
	dma->vaddr.ladr_32	= PLX_VADDR(dev, 0x88);
	dma->vaddr.siz_32	= PLX_VADDR(dev, 0x8C);
	dma->vaddr.dpr_32	= PLX_VADDR(dev, 0x90);
	dma->vaddr.csr_8	= PLX_VADDR(dev, 0xA8);
	_dma_channel_reset(dev, dma);

	dma	= &dev->dma.channel[1];
	dma->index			= 1;
	dma->flags			= ch1_flags;
	dma->intcsr_enable	= GSC_INTCSR_DMA_1_INT_ENABLE;
	dma->vaddr.mode_32	= PLX_VADDR(dev, 0x94);
	dma->vaddr.padr_32	= PLX_VADDR(dev, 0x98);
	dma->vaddr.ladr_32	= PLX_VADDR(dev, 0x9C);
	dma->vaddr.siz_32	= PLX_VADDR(dev, 0xA0);
	dma->vaddr.dpr_32	= PLX_VADDR(dev, 0xA4);
	dma->vaddr.csr_8	= PLX_VADDR(dev, 0xA9);
	_dma_channel_reset(dev, dma);

	return(0);
}



/******************************************************************************
*
*	Function:	gsc_dma_destroy
*
*	Purpose:
*
*		Perform a one time clean up of the DMA portion of the given structure.
*
*	Arguments:
*
*		dev		The data for the device of interest.
*
*	Returned:
*
*		None.
*
******************************************************************************/

void gsc_dma_destroy(dev_data_t* dev)
{
	_dma_channel_reset(dev, &dev->dma.channel[0]);
	_dma_channel_reset(dev, &dev->dma.channel[1]);
	gsc_sem_destroy(&dev->dma.sem);
}



/******************************************************************************
*
*	Function:	gsc_dma_open
*
*	Purpose:
*
*		Perform the steps needed to prepare the DMA code for operation due to
*		the device being opened.
*
*	Arguments:
*
*		dev		The data for the device of interest.
*
*	Returned:
*
*		0		All went well.
*		< 0		The code for the problem seen.
*
******************************************************************************/

int gsc_dma_open(dev_data_t* dev)
{
	_dma_channel_reset(dev, &dev->dma.channel[0]);
	_dma_channel_reset(dev, &dev->dma.channel[1]);
	return(0);
}



/******************************************************************************
*
*	Function:	gsc_dma_perform
*
*	Purpose:
*
*		Perform a DMA transfer per the given arguments.
*
*		NOTES:
*
*		With interrupts it is possible for the requested interrupt to occure
*		before the current task goes to sleep. This is possible, for example,
*		because the DMA transfer could complete before the task gets to sleep.
*		So, to make sure things work in these cases, we take manual steps to
*		insure that the current task can be awoken by the interrupt before the
*		task actually sleeps and before the interrupt is enabled.
*
*	Arguments:
*
*		dev			The device data structure.
*
*		io			The I/O structure for the operation to perform.
*
*		jif_end		Timeout at this point in time (in jiffies).
*
*		ability		The required read/write ability.
*
*		mode		The value for the DMAMODE register.
*
*		dpr			The necessary bits for the DMADPR register.
*
*		buff		The data transfer buffer.
*
*		bytes		The number of bytes to transfer.
*
*	Returned:
*
*		>= 0		The number of bytes transferred.
*		< 0			There was an error.
*
******************************************************************************/

ssize_t gsc_dma_perform(
	dev_data_t*			dev,
	dev_io_t*			io,
	unsigned long		jif_end,
	unsigned int		ability,
	u32					mode,
	u32					dpr,
	void*				buff,
	ssize_t				bytes)
{
	u8					csr;
	gsc_dma_ch_t*		dma;
	int					i;
	ssize_t				qty;
	ssize_t				ret;
	long				timeout;
	u32					ul;
	gsc_wait_t			wt;
	gsc_sem_t			sem;

	gsc_sem_create(&sem);	// dummy required for wait operations.

	for (;;)	// Use a loop for convenience.
	{
		dma	= _dma_channel_get(dev, io, ability);

		if (dma == NULL)
		{
			// We couldn't get a DMA channel.
			qty	= -EAGAIN;
			break;
		}

		io->dma_channel	= dma;

		// Enable the DMA interrupt.
		gsc_irq_intcsr_mod(dev, dma->intcsr_enable, dma->intcsr_enable);

		// DMAMODEx
		writel(mode, dma->vaddr.mode_32);

		// DMAPADRx
		ul	= virt_to_bus(buff);
		writel(ul, dma->vaddr.padr_32);

		// DMALADRx
		writel(io->io_reg_offset, dma->vaddr.ladr_32);

		// DMASIZx
		writel(bytes, dma->vaddr.siz_32);

		// DMADPRx
		writel(dpr, dma->vaddr.dpr_32);

		// DMAARB
		writel(0, dev->vaddr.plx_dmaarb_32);

		// DMATR
		writel(0, dev->vaddr.plx_dmathr_32);

		//	Wait for completion.

		if (io->non_blocking)
		{
			// Allow at least one second for the DMA.
			timeout	= 1000;
		}
		else
		{
			timeout	= gsc_time_delta(jiffies, jif_end);

			if (timeout < 10)
				timeout	= 10;
			else
				timeout	= -timeout;	// report timeout in jiffies.
		}

		memset(&wt, 0, sizeof(wt));
		wt.flags		= GSC_WAIT_FLAG_INTERNAL;
		wt.main			= dma->index ? GSC_WAIT_MAIN_DMA1 : GSC_WAIT_MAIN_DMA0;
		wt.timeout_ms	= timeout;
		ret	= gsc_wait_event(dev, &wt, _dma_start, (unsigned long) io, &sem);

		// Clean up.

		// Check the results.
		csr	= readb(dma->vaddr.csr_8);

		// Disable the DONE interrupt.
		ul	= readl(dma->vaddr.mode_32);
		ul	&= ~GSC_DMA_MODE_INTERRUPT_WHEN_DONE;
		writel(ul, dma->vaddr.mode_32);


		if ((csr & GSC_DMA_CSR_DONE) == 0)
		{
			//	If it isn't done then the operation timed out.
			//	Unfortunately the PLX PCI9080 doesn't tell us
			//	how many bytes were transferred. So, instead
			//	of reporting the number of transferred bytes,
			//	or zero, we return an error status.

			// Force DMA termination.
			// If the DMA has, by chance, already ended, then this abort
			// request will apply to the next DMA request on this channel.
			writeb(GSC_DMA_CSR_ABORT, dma->vaddr.csr_8);
			udelay(10);		// Wait for completion.
			qty	= -ETIMEDOUT;
		}
		else
		{
			qty	= bytes;
		}

		// Disable the DMA interrupt, just in case.
		gsc_irq_intcsr_mod(dev, 0, dma->intcsr_enable);

		// Clear the DMA.
		writeb(GSC_DMA_CSR_CLEAR, dma->vaddr.csr_8);

		if (dma->flags & GSC_DMA_SEL_DYNAMIC)
		{
			i				= gsc_sem_lock(&dev->dma.sem);
			io->dma_channel	= NULL;
			dma->in_use		= 0;

			if (i == 0)
				gsc_sem_unlock(&dev->dma.sem);
		}

		break;
	}

	gsc_sem_destroy(&sem);
	return(qty);
}



/******************************************************************************
*
*	Function:	gsc_dma_abort_active_xfer
*
*	Purpose:
*
*		Abort the I/O channel's DMA transfer if it is in progress.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		io		The I/O structure for the operation to access.
*
*	Returned:
*
*		1		An active DMA was aborted.
*		0		A DMA was not in progress.
*
******************************************************************************/

int gsc_dma_abort_active_xfer(dev_data_t* dev, dev_io_t* io)
{
	u8				csr;
	gsc_dma_ch_t*	dma;
	int				i;
	int				ret	= 0;

	i	= gsc_sem_lock(&dev->dma.sem);
	dma	= io->dma_channel;

	if ((dma) && (dma->in_use))
	{
		csr	= readb(dma->vaddr.csr_8);

		if ((csr & GSC_DMA_CSR_DONE) == 0)
		{
			// Force DMA termination.
			// If the DMA has, by chance, already ended, then this abort
			// request will apply to the next DMA request on this channel.
			writeb(GSC_DMA_CSR_ABORT, dma->vaddr.csr_8);
			udelay(10);		// Wait for completion.
			ret	= 1;
		}
	}

	if (i == 0)
		gsc_sem_unlock(&dev->dma.sem);

	return(ret);
}


