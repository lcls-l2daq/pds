// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_read.c $
// $Rev$
// $Date$

#include "gsc_main.h"
#include "main.h"



// #defines	*******************************************************************

#define	_RX_ABORT	GSC_WAIT_IO_RX_ABORT
#define	_RX_DONE	GSC_WAIT_IO_RX_DONE
#define	_RX_ERROR	GSC_WAIT_IO_RX_ERROR
#define	_RX_TIMEOUT	GSC_WAIT_IO_RX_TIMEOUT



/******************************************************************************
*
*	Function:	_read_work
*
*	Purpose:
*
*		Implement the mode independent working portion of the read() procedure.
*		If a timeout is permitted and called for, we wait a single timer tick
*		and check again.
*
*	Arguments:
*
*		dev				The device data structure.
*
*		buf				The data read is placed here.
*
*		count			The number of bytes requested. This is positive.
*
*		jif_end			Timeout at this point in time (in jiffies).
*
*		fn_available	This is the mode specific function that determines the
*						number of bytes that may be read.
*
*		fn_work			This is the function implementing the mode specific
*						reading functionality.
*
*	Returned:
*
*		>= 0			This is the number of bytes read.
*		< 0				There was a problem and this is the error status.
*
******************************************************************************/

#if defined(DEV_SUPPORTS_READ)
static ssize_t _read_work(
	dev_data_t*		dev,
	char*			buf,
	ssize_t			count,	// bytes
	unsigned long	jif_end,
	ssize_t			(*fn_available)(
						dev_data_t*		dev,
						ssize_t			count),	// bytes
	ssize_t			(*fn_work)(
						dev_data_t*		dev,
						char*			buf,
						ssize_t			count,	// bytes
						unsigned long	jif_end))
{
	ssize_t			get;			// Number of bytes to get this time.
	ssize_t			got;			// Number of bytes we got this time.
	ssize_t			total	= 0;	// The number of bytes transferred.
	unsigned long	ul;

	for (;;)	// Continue until we've got to stop.
	{
		// See how much data we can transfer.
		get	= (fn_available)(dev, count);

		if (get > dev->rx.bytes)
			get	= dev->rx.bytes;	// The size of the transfer buffer.

		if (get > count)
			get	= count;			// Try to complete the request.

		// Either transfer the data or see what to do next.

		if (dev->rx.abort)
		{
			// We've been told to quit.
			dev->rx.abort	= 0;
			gsc_wait_resume_io(dev, _RX_DONE | _RX_ABORT);
			break;
		}

		if (get > 0)
			got	= (fn_work)(dev, dev->rx.buf, get, jif_end);
		else
			got	= 0;

		if (got < 0)
		{
			// There was a problem.
			total	= got;
			gsc_wait_resume_io(dev, _RX_DONE | _RX_ERROR);
			break;
		}
		else if (got > 0)
		{
			// Copy the data to the application buffer.
			ul	= copy_to_user(buf, dev->rx.buf, got);

			if (ul)
			{
				// We couldn't copy out the data.
				total	= -EFAULT;
				gsc_wait_resume_io(dev, _RX_DONE | _RX_ERROR);
				break;
			}

			// House keeping.
			count	-= got;
			total	+= got;
			buf		+= got;
		}

		// Now what do we do?

		if (count == 0)
		{
			// The request has been fulfilled.
			gsc_wait_resume_io(dev, _RX_DONE);
			break;
		}
		else if (dev->rx.non_blocking)
		{
			// We can't block.

			if (got > 0)
			{
				// See if we can get more data.
				continue;
			}
			else
			{
				// We can't wait for data to arrive.
				// We have essentially timed out.
				gsc_wait_resume_io(dev, _RX_DONE | _RX_TIMEOUT);
				break;
			}
		}
		else if (JIFFIES_TIMEOUT(jif_end))
		{
			// We've timed out.
			gsc_wait_resume_io(dev, _RX_DONE | _RX_TIMEOUT);
			break;
		}
		else if (got)
		{
			// Some data was available, so go back for more.
			continue;
		}

		if (dev->rx.abort)
		{
			// We've been told to quit.
			dev->rx.abort	= 0;
			gsc_wait_resume_io(dev, _RX_DONE | _RX_ABORT);
			break;
		}

		// Wait for 1 timer tick before checking again.
		SET_CURRENT_STATE(TASK_INTERRUPTIBLE);
		schedule_timeout(1);
		SET_CURRENT_STATE(TASK_RUNNING);
	}

	return(total);
}
#endif



/******************************************************************************
*
*	Function:	gsc_read
*
*	Purpose:
*
*		Implement the read() procedure. Only one read thread is permitted at a
*		time.
*
*	Arguments:
*
*		filp	This is the file structure.
*
*		buf		The data requested goes here.
*
*		count	The number of bytes requested.
*
*		offp	The file position the user is accessing.
*
*	Returned:
*
*		0		All went well, though no data was transferred.
*		> 0		This is the number of bytes transferred.
*		< 0		There was an error.
*
******************************************************************************/

ssize_t gsc_read(
	struct file*	filp,
	char*			buf,
	size_t			count,	// bytes
	loff_t*			offp)
{
#if defined(DEV_SUPPORTS_READ)

	dev_data_t*		dev;
	unsigned long	jif_end;
	ssize_t			ret;
	size_t			samples;
	int				test;

	for (;;)	// We'll use a loop for convenience.
	{
		// Access the device structure.
		dev	= (dev_data_t*) filp->private_data;

		if (dev == NULL)
		{
			// The referenced device doesn't exist.
			ret	= -ENODEV;
			break;
		}

		// Gain exclusive access to the device structure.
		ret	= gsc_sem_lock(&dev->sem);

		if (ret)
		{
			// We got a signal rather than the semaphore.
			break;
		}

		// Perform argument validation.

		if (count <= 0)
		{
			// There is no work to do.
			ret	= 0;
			gsc_wait_resume_io(dev, _RX_DONE);
			gsc_sem_unlock(&dev->sem);
			break;
		}

		if (dev->rx.bytes_per_sample == 4)
		{
		}
		else if (dev->rx.bytes_per_sample == 2)
		{
		}
		else if (dev->rx.bytes_per_sample == 1)
		{
		}
		else
		{
			// This is an internal error.
			ret	= -EINVAL;
			gsc_wait_resume_io(dev, _RX_DONE | _RX_ERROR);
			gsc_sem_unlock(&dev->sem);
			break;
		}

		if (count % dev->rx.bytes_per_sample)
		{
			// Requests must be in sample size increments.
			ret	= -EINVAL;
			gsc_wait_resume_io(dev, _RX_DONE | _RX_ERROR);
			gsc_sem_unlock(&dev->sem);
			break;
		}

		if (buf == NULL)
		{
			// No buffer provided.
			ret	= -EINVAL;
			gsc_wait_resume_io(dev, _RX_DONE | _RX_ERROR);
			gsc_sem_unlock(&dev->sem);
			break;
		}

		test	= access_ok(VERIFY_WRITE, buf, count);

		if (test == 0)
		{
			// We can't write to the user's memory.
			ret	= -EFAULT;
			gsc_wait_resume_io(dev, _RX_DONE | _RX_ERROR);
			gsc_sem_unlock(&dev->sem);
			break;
		}

		// Compute the I/O timeout end.
		jif_end	= jiffies + dev->rx.timeout_s * HZ;

		// Is this blocking or non-blocking I/O.
		dev->rx.non_blocking	= (filp->f_flags & O_NONBLOCK) ? 1 : 0;

		// Transfer access control to the read semaphore.
		ret	= gsc_sem_lock(&dev->rx.sem);

		if (ret)
		{
			// We got a signal rather than the read semaphore.
			gsc_wait_resume_io(dev, _RX_DONE | _RX_ERROR);
			gsc_sem_unlock(&dev->sem);
			break;
		}

		gsc_sem_unlock(&dev->sem);
		ret	= dev_read_startup(dev);

		if (ret)
		{
			// There was a problem.
			gsc_wait_resume_io(dev, _RX_DONE | _RX_ERROR);
			gsc_sem_unlock(&dev->rx.sem);
			break;
		}

		// Perform the operation.
		samples	= count / dev->rx.bytes_per_sample;

		if ((samples <= dev->rx.pio_threshold) ||
			(dev->rx.io_mode == GSC_IO_MODE_PIO))
		{
			ret	= _read_work(	dev,
								buf,
								count,	// bytes
								jif_end,
								DEV_PIO_READ_AVAILABLE,
								DEV_PIO_READ_WORK);
		}
		else if (dev->rx.io_mode == GSC_IO_MODE_DMA)
		{
			ret	= _read_work(	dev,
								buf,
								count,	// bytes
								jif_end,
								DEV_DMA_READ_AVAILABLE,
								DEV_DMA_READ_WORK);
		}
		else if (dev->rx.io_mode == GSC_IO_MODE_DMDMA)
		{
			ret	= _read_work(	dev,
								buf,
								count,	// bytes
								jif_end,
								DEV_DMDMA_READ_AVAILABLE,
								DEV_DMDMA_READ_WORK);
		}
		else
		{
			ret	= -EINVAL;
			gsc_wait_resume_io(dev, _RX_DONE | _RX_ERROR);
		}

		//	Clean up.

		gsc_sem_unlock(&dev->rx.sem);
		break;
	}

	return(ret);
#else
    return (-EPERM);
#endif
}



/******************************************************************************
*
*	Function:	gsc_read_abort_active_xfer
*
*	Purpose:
*
*		Abort an active read.
*
*	Arguments:
*
*		dev		The device data structure.
*
*	Returned:
*
*		1		An active read was aborted.
*		0		A read was not in progress.
*
******************************************************************************/

int gsc_read_abort_active_xfer(dev_data_t* dev)
{
#if defined(DEV_SUPPORTS_READ)

	int	i;
	int	ret;

	dev->rx.abort	= 1;
	ret				= gsc_dma_abort_active_xfer(dev, &dev->rx);
	i				= gsc_sem_lock(&dev->rx.sem);
	ret				= ret ? ret : (dev->rx.abort ? 0 : 1);
	dev->rx.abort	= 0;

	if (i == 0)
		gsc_sem_unlock(&dev->rx.sem);

	return(ret);
#endif
	return(0);
}



/******************************************************************************
*
*	Function:	gsc_read_pio_work_8_bit
*
*	Purpose:
*
*		Perform PIO based reads of 8-bit values.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		buff	The destination for the data read.
*
*		count	The number of bytes to read.
*
*		jif_end	The "jiffies" time at which we timeout.
*
*	Returned:
*
*		>= 0	The number of bytes written.
*		< 0		There was an error.
*
******************************************************************************/

#if defined(DEV_SUPPORTS_READ)
#if defined(GSC_READ_PIO_WORK) || defined(GSC_READ_PIO_WORK_8_BIT)
ssize_t gsc_read_pio_work_8_bit(
	dev_data_t*		dev,
	char*			buff,
	ssize_t			count,
	unsigned long	jif_end)
{
	ssize_t	qty	= count;
	VADDR_T	src	= dev->rx.io_reg_vaddr;
	u8*		dst	= (u8*) buff;

	for (; count > 0; )
	{
		count	-= 1;
		dst[0]	= readb(src);
		dst++;

		if (dev->rx.timeout_s)
		{
			if (JIFFIES_TIMEOUT(jif_end))
			{
				// We've timed out.
				break;
			}
		}
	}

	qty	-= count;
	return(qty);
}
#endif
#endif



/******************************************************************************
*
*	Function:	gsc_read_pio_work_16_bit
*
*	Purpose:
*
*		Perform PIO based reads of 16-bit values.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		buff	The destination for the data read.
*
*		count	The number of bytes to read.
*
*		jif_end	The "jiffies" time at which we timeout.
*
*	Returned:
*
*		>= 0	The number of bytes written.
*		< 0		There was an error.
*
******************************************************************************/

#if defined(DEV_SUPPORTS_READ)
#if defined(GSC_READ_PIO_WORK) || defined(GSC_READ_PIO_WORK_16_BIT)
ssize_t gsc_read_pio_work_16_bit(
	dev_data_t*		dev,
	char*			buff,
	ssize_t			count,
	unsigned long	jif_end)
{
	ssize_t	qty	= count;
	VADDR_T	src	= dev->rx.io_reg_vaddr;
	u16*	dst	= (u16*) buff;

	for (; count > 0; )
	{
		count	-= 2;
		dst[0]	= readw(src);
		dst++;

		if (dev->rx.timeout_s)
		{
			if (JIFFIES_TIMEOUT(jif_end))
			{
				// We've timed out.
				break;
			}
		}
	}

	qty	-= count;
	return(qty);
}
#endif
#endif



/******************************************************************************
*
*	Function:	gsc_read_pio_work_32_bit
*
*	Purpose:
*
*		Perform PIO based reads of 32-bit values.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		buff	The destination for the data read.
*
*		count	The number of bytes to read.
*
*		jif_end	The "jiffies" time at which we timeout.
*
*	Returned:
*
*		>= 0	The number of bytes written.
*		< 0		There was an error.
*
******************************************************************************/

#if defined(DEV_SUPPORTS_READ)
#if defined(GSC_READ_PIO_WORK) || defined(GSC_READ_PIO_WORK_32_BIT)
ssize_t gsc_read_pio_work_32_bit(
	dev_data_t*		dev,
	char*			buff,
	ssize_t			count,
	unsigned long	jif_end)
{
	ssize_t	qty	= count;
	VADDR_T	src	= dev->rx.io_reg_vaddr;
	u32*	dst	= (u32*) buff;

	for (; count > 0; )
	{
		count	-= 4;
		dst[0]	= readl(src);
		dst++;
			if (dev->rx.timeout_s)
		{
			if (JIFFIES_TIMEOUT(jif_end))
			{
				// We've timed out.
				break;
			}
		}
	}

	qty	-= count;
	return(qty);
}
#endif
#endif



/******************************************************************************
*
*	Function:	gsc_read_pio_work
*
*	Purpose:
*
*		Perform PIO based reads of 32, 16 or 8 bit values.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		buff	The destination for the data read.
*
*		count	The number of bytes to read.
*
*		jif_end	The "jiffies" time at which we timeout.
*
*	Returned:
*
*		>= 0	The number of bytes written.
*		< 0		There was an error.
*
******************************************************************************/

#if defined(DEV_SUPPORTS_READ)
#if defined(GSC_READ_PIO_WORK)
ssize_t gsc_read_pio_work(
	dev_data_t*		dev,
	char*			buff,
	ssize_t			count,
	unsigned long	jif_end)
{
	ssize_t	qty;

	if (dev->rx.data_size_bits == 32)
		qty	= gsc_read_pio_work_32_bit(dev, buff, count, jif_end);
	else if (dev->rx.data_size_bits == 16)
		qty	= gsc_read_pio_work_16_bit(dev, buff, count, jif_end);
	else if (dev->rx.data_size_bits == 8)
		qty	= gsc_read_pio_work_8_bit(dev, buff, count, jif_end);
	else
		qty	= -EINVAL;

	return(qty);
}
#endif
#endif


