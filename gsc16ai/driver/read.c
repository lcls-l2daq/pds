// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/LINUX/16AI32SSC/driver/read.c $
// $Rev: 6276 $
// $Date$

#include "main.h"



/******************************************************************************
*
*	Function:	dev_read_startup
*
*	Purpose:
*
*		Perform any work or tests needed before initiating a read operation.
*
*	Arguments:
*
*		dev		The device data structure.
*
*	Returned:
*
*		0		All is well.
*		< 0		There was an error.
*
******************************************************************************/

ssize_t dev_read_startup(dev_data_t* dev)
{
	u32		ibcr;
	ssize_t	ret	= 0;

	if (dev->rx.overflow_check)
	{
		ibcr	= readl(dev->vaddr.gsc_bctlr_32);

		if (ibcr & 0x20000)
			ret	= -EIO;
	}

	if (dev->rx.underflow_check)
	{
		ibcr	= readl(dev->vaddr.gsc_bctlr_32);

		if (ibcr & 0x10000)
			ret	= -EIO;
	}

	return(ret);
}



/******************************************************************************
*
*	Function:	pio_read_available
*
*	Purpose:
*
*		Check to see how many bytes are available for reading.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		count	What remains of the number of bytes that the application
*				requested.
*
*	Returned:
*
*		>= 0	The number of bytes available for reading.
*		< 0		There was an error.
*
******************************************************************************/

ssize_t pio_read_available(dev_data_t* dev, ssize_t count)
{
	u32	bsr;

	bsr		= readl(dev->vaddr.gsc_bufsr_32);
	count	= 4 * bsr;
	return(count);
}



/******************************************************************************
*
*	Function:	dma_read_available
*
*	Purpose:
*
*		Check to see how many bytes are available for reading.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		count	What remains of the number of bytes that the application
*				requested.
*
*	Returned:
*
*		>= 0	The number of bytes available for reading.
*		< 0		There was an error.
*
******************************************************************************/

ssize_t dma_read_available(dev_data_t* dev, ssize_t count)
{
	u32	available;
	u32	bsr;
	u32	ibcr;
	u32	threshold;

	bsr			= readl(dev->vaddr.gsc_bufsr_32);
	available	= bsr * 4;

	if (available >= count)
	{
		// The desired amount is available.
	}
	else
	{
		ibcr		= readl(dev->vaddr.gsc_ibcr_32);
		threshold	= (ibcr & 0x3FFFF) * 4;

		if (available >= threshold)
			count	= available;
		else
			count	= 0;
	}

	return(count);
}



/******************************************************************************
*
*	Function:	dma_read_work
*
*	Purpose:
*
*		Perform a read of the specified number of bytes.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		buff	Put the data here.
*
*		count	What remains of the number of bytes that the application
*				requested.
*
*		jif_end	The "jiffies" time at which we timeout. If this is zero, then
*				we ignore this.
*
*	Returned:
*
*		>= 0	The number of bytes read.
*		< 0		There was an error.
*
******************************************************************************/

ssize_t dma_read_work(
	dev_data_t*		dev,
	char*			buf,
	ssize_t			count,
	unsigned long	jif_end)
{
	u32		dpr;
	u32		mode;
	ssize_t	qty;
	ssize_t	samples	= count / 4;

	if (samples < dev->rx.pio_threshold)
	{
		qty	= gsc_read_pio_work_32_bit(dev, buf, count, jif_end);
	}
	else
	{
		mode	= GSC_DMA_MODE_BLOCK_DMA
				| GSC_DMA_MODE_SIZE_32_BITS
				| GSC_DMA_MODE_INPUT_ENABLE
				| GSC_DMA_MODE_BURSTING_LOCAL
				| GSC_DMA_MODE_INTERRUPT_WHEN_DONE
				| GSC_DMA_MODE_LOCAL_ADRESS_CONSTANT
				| GSC_DMA_MODE_PCI_INTERRUPT_ENABLE;

		dpr		= GSC_DMA_DPR_BOARD_TO_HOST
				| GSC_DMA_DPR_END_OF_CHAIN
				| GSC_DMA_DPR_TERMINAL_COUNT_IRQ;

		qty		= gsc_dma_perform(	dev,
									&dev->rx,
									jif_end,
									GSC_DMA_CAP_DMA_READ,
									mode,
									dpr,
									buf,
									count);
	}

	return(qty);
}



/******************************************************************************
*
*	Function:	dmdma_read_available
*
*	Purpose:
*
*		Check to see how many bytes are available for reading.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		count	What remains of the number of bytes that the application
*				requested.
*
*	Returned:
*
*		>= 0	The number of bytes available for reading.
*		< 0		There was an error.
*
******************************************************************************/

ssize_t dmdma_read_available(dev_data_t* dev, ssize_t count)
{
	u32	ibcr;
	u32	threshold;

	ibcr		= readl(dev->vaddr.gsc_ibcr_32);
	threshold	= ibcr & 0x3FFFF;
	count		= (threshold + 1) * 4;
	return(count);
}



/******************************************************************************
*
*	Function:	dmdma_read_work
*
*	Purpose:
*
*		Perform a read of the specified number of bytes.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		buff	Put the data here.
*
*		count	What remains of the number of bytes that the application
*				requested.
*
*		jif_end	The "jiffies" time at which we timeout. If this is zero, then
*				we ignore this.
*
*	Returned:
*
*		>= 0	The number of bytes read.
*		< 0		There was an error.
*
******************************************************************************/

ssize_t dmdma_read_work(
	dev_data_t*		dev,
	char*			buf,
	ssize_t			count,
	unsigned long	jif_end)
{
	u32		dpr;
	u32		mode;
	ssize_t	qty;
	ssize_t	samples	= count / 4;

	if (samples < dev->rx.pio_threshold)
	{
		qty	= gsc_read_pio_work_32_bit(dev, buf, count, jif_end);
	}
	else
	{
		mode	= GSC_DMA_MODE_DM_DMA
				| GSC_DMA_MODE_SIZE_32_BITS
				| GSC_DMA_MODE_INPUT_ENABLE
				| GSC_DMA_MODE_BURSTING_LOCAL
				| GSC_DMA_MODE_INTERRUPT_WHEN_DONE
				| GSC_DMA_MODE_LOCAL_ADRESS_CONSTANT
				| GSC_DMA_MODE_PCI_INTERRUPT_ENABLE;

		dpr		= GSC_DMA_DPR_BOARD_TO_HOST
				| GSC_DMA_DPR_END_OF_CHAIN
				| GSC_DMA_DPR_TERMINAL_COUNT_IRQ;

		qty		= gsc_dma_perform(	dev,
									&dev->rx,
									jif_end,
									GSC_DMA_CAP_DMDMA_READ,
									mode,
									dpr,
									buf,
									count);
	}

	return(qty);
}


