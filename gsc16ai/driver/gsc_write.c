// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_write.c $
// $Rev: 6255 $
// $Date$

#include "gsc_main.h"
#include "main.h"



// #defines	*******************************************************************

#define	_TX_ABORT	GSC_WAIT_IO_TX_ABORT
#define	_TX_DONE	GSC_WAIT_IO_TX_DONE
#define	_TX_ERROR	GSC_WAIT_IO_TX_ERROR
#define	_TX_TIMEOUT	GSC_WAIT_IO_TX_TIMEOUT



/******************************************************************************
*
*	Function:	_write_work
*
*	Purpose:
*
*		Implement the mode independent working portion of the write()
*		procedure. If a timeout is permitted and called for, we wait a single
*		timer tick and check again.
*
*	Arguments:
*
*		dev				The board data structure.
*
*		buf				The bytes to write come from here.
*
*		count			The number of bytes requested. This is positive.
*
*		jif_end			Timeout at this point in time (in jiffies).
*
*		fn_available	This is the mode specific function that determines the
*						number of bytes that may be written.
*
*		fn_work			This is the function implementing the mode specific
*						write functionality.
*
*	Returned:
*
*		>= 0			This is the number of bytes written.
*		< 0				An appropriate error status.
*
******************************************************************************/

#if defined(DEV_SUPPORTS_WRITE)
static ssize_t _write_work(
	dev_data_t*		dev,
	const char*		buf,
	ssize_t			count,	// bytes
	unsigned long	jif_end,
	ssize_t			(*fn_available)(
						dev_data_t*		dev,
						ssize_t			count),	// bytes
	ssize_t			(*fn_work)(
						dev_data_t*		dev,
						const char*		buf,
						ssize_t			count,	// bytes
						unsigned long	jif_end))
{
	ssize_t			total	= 0;	// Total bytes written to the board.
	ssize_t			want	= 0;	// Bytes we want to write this time.
	long			send;
	long			sent;
	char*			src		= dev->tx.buf;
	unsigned long	ul;

	for (;;)
	{
		if (want <= 0)
		{
			// Transfer one block at a time.

			if (count > dev->tx.bytes)
				want	= dev->tx.bytes;
			else
				want	= count;

			src		= dev->tx.buf;
			ul		= copy_from_user(src, buf, want);
			buf		+= want;

			if (ul)
			{
				// We couldn't get user data.
				total	= -EFAULT;
				gsc_wait_resume_io(dev, _TX_DONE | _TX_ERROR);
				break;
			}
		}

		// See how much data we can transfer.
		send	= (fn_available)(dev, count);

		if (send > dev->tx.bytes)
			send	= dev->tx.bytes;	// The size of the transfer buffer.

		if (send > count)
			send	= count;	// Try to complete the request.

		// Either transfer the data or see what to do next.

		if (dev->tx.abort)
		{
			// We've been told to quit.
			dev->tx.abort	= 0;
			gsc_wait_resume_io(dev, _TX_DONE | _TX_ABORT);
			break;
		}

		if (send > 0)
			sent	= (fn_work)(dev, src, send, jif_end);
		else
			sent	= 0;

		if (sent < 0)
		{
			// There was a problem.
			total	= sent;
			gsc_wait_resume_io(dev, _TX_DONE | _TX_ERROR);
			break;
		}
		else if (sent > 0)
		{
			// House keeping.
			count	-= sent;
			total	+= sent;
			src		+= sent;
			want	-= sent;
		}

		// Now what do we do?

		if (count == 0)
		{
			// The request has been fulfilled.
			gsc_wait_resume_io(dev, _TX_DONE);
			break;
		}
		else if (dev->tx.non_blocking)
		{
			// We can't block.

			if (sent > 0)
			{
				// See if we can send some more data.
				continue;
			}
			else
			{
				// We can't wait for space to write to.
				// We have essentially timed out.
				gsc_wait_resume_io(dev, _TX_DONE | _TX_TIMEOUT);
				break;
			}
		}
		else if (JIFFIES_TIMEOUT(jif_end))
		{
			// We've timed out.
			gsc_wait_resume_io(dev, _TX_DONE | _TX_TIMEOUT);
			break;
		}
		else if (sent)
		{
			// Some data was written, so go back for more.
			continue;
		}

		if (dev->tx.abort)
		{
			// We've been told to quit.
			dev->tx.abort	= 0;
			gsc_wait_resume_io(dev, _TX_DONE | _TX_ABORT);
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
*	Function:	write
*
*	Purpose:
*
*		Perform a PIO based write.
*
*	Arguments.
*
*		filp	This is the file structure.
*
*		buf		The data requested goes here.
*
*		count	The number of bytes requested.
*
*		offp	The file position the user is requesting.
*
*	Returned:
*
*		>= 0	The number of bytes writen.
*		< 0		An appropriate error code.
*
******************************************************************************/

ssize_t gsc_write(
	struct file*	filp,
	const char*		buf,
	size_t			count,
	loff_t*			offp)
{
#if defined(DEV_SUPPORTS_WRITE)

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
			gsc_wait_resume_io(dev, _TX_DONE);
			gsc_sem_unlock(&dev->sem);
			break;
		}

		if (dev->tx.bytes_per_sample == 4)
		{
		}
		else if (dev->tx.bytes_per_sample == 2)
		{
		}
		else if (dev->tx.bytes_per_sample == 1)
		{
		}
		else
		{
			// This is an internal error.
			ret	= -EINVAL;
			gsc_wait_resume_io(dev, _TX_DONE | _TX_ERROR);
			gsc_sem_unlock(&dev->sem);
			break;
		}

		if (count % dev->tx.bytes_per_sample)
		{
			// Requests must be in sample size increments.
			ret	= -EINVAL;
			gsc_wait_resume_io(dev, _TX_DONE | _TX_ERROR);
			gsc_sem_unlock(&dev->sem);
			break;
		}

		if (buf == NULL)
		{
			// No buffer provided.
			ret	= -EINVAL;
			gsc_wait_resume_io(dev, _TX_DONE | _TX_ERROR);
			gsc_sem_unlock(&dev->sem);
			break;
		}

		test	= access_ok(VERIFY_READ, buf, count);

		if (test == 0)
		{
			// We can't write to the user's memory.
			ret	= -EFAULT;
			gsc_wait_resume_io(dev, _TX_DONE | _TX_ERROR);
			gsc_sem_unlock(&dev->sem);
			break;
		}

		// Compute the I/O timeout end.
		jif_end	= jiffies + dev->tx.timeout_s * HZ;

		// Is this blocking or non-blocking I/O.
		dev->tx.non_blocking	= (filp->f_flags & O_NONBLOCK) ? 1 : 0;

		// Transfer access control to the read semaphore.
		ret	= gsc_sem_lock(&dev->tx.sem);

		if (ret)
		{
			// We got a signal rather than the read semaphore.
			gsc_wait_resume_io(dev, _TX_DONE | _TX_ERROR);
			gsc_sem_unlock(&dev->sem);
			break;
		}

		gsc_sem_unlock(&dev->sem);
		ret	= dev_write_startup(dev);

		if (ret)
		{
			// There was a problem.
			gsc_wait_resume_io(dev, _TX_DONE | _TX_ERROR);
			gsc_sem_unlock(&dev->tx.sem);
			break;
		}

		// Perform the operation.
		samples	= count / dev->tx.bytes_per_sample;

		if ((samples <= dev->tx.pio_threshold) ||
			(dev->tx.io_mode == GSC_IO_MODE_PIO))
		{
			ret	= _write_work(	dev,
								buf,
								count,
								jif_end,
								DEV_PIO_WRITE_AVAILABLE,
								DEV_PIO_WRITE_WORK);
		}
		else if (dev->tx.io_mode == GSC_IO_MODE_DMA)
		{
			ret	= _write_work(	dev,
								buf,
								count,
								jif_end,
								DEV_DMA_WRITE_AVAILABLE,
								DEV_DMA_WRITE_WORK);
		}
		else if (dev->tx.io_mode == GSC_IO_MODE_DMDMA)
		{
			ret	= _write_work(	dev,
								buf,
								count,
								jif_end,
								DEV_DMDMA_WRITE_AVAILABLE,
								DEV_DMDMA_WRITE_WORK);
		}
		else
		{
			ret	= -EINVAL;
			gsc_wait_resume_io(dev, _TX_DONE | _TX_ERROR);
		}

		//	Clean up.

		gsc_sem_unlock(&dev->tx.sem);
		break;
	}

	return(ret);
#else
    return (-EPERM);
#endif
}



/******************************************************************************
*
*	Function:	gsc_write_abort_active_xfer
*
*	Purpose:
*
*		Abort an active write.
*
*	Arguments:
*
*		dev		The device data structure.
*
*	Returned:
*
*		1		An active write was aborted.
*		0		A warite was not in progress.
*
******************************************************************************/

int gsc_write_abort_active_xfer(dev_data_t* dev)
{
#if defined(DEV_SUPPORTS_WRITE)

	int	i;
	int	ret;

	dev->tx.abort	= 1;
	ret				= gsc_dma_abort_active_xfer(dev, &dev->tx);
	i				= gsc_sem_lock(&dev->tx.sem);
	ret				= ret ? ret : (dev->tx.abort ? 0 : 1);
	dev->tx.abort	= 0;

	if (i == 0)
		gsc_sem_unlock(&dev->tx.sem);

	return(ret);
#endif
	return(0);
}



/******************************************************************************
*
*	Function:	gsc_write_pio_work_8_bit
*
*	Purpose:
*
*		Perform PIO based writes of 8-bit values.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		buff	The source for the data to write.
*
*		count	The number of bytes to write.
*
*		jif_end	The "jiffies" time at which we timeout.
*
*	Returned:
*
*		>= 0	The number of bytes written.
*		< 0		There was an error.
*
******************************************************************************/

#if defined(DEV_SUPPORTS_WRITE)
#if defined(GSC_WRITE_PIO_WORK) || defined(GSC_WRITE_PIO_WORK_8_BIT)
ssize_t gsc_write_pio_work_8_bit(
	dev_data_t*		dev,
	const char*		buff,
	ssize_t			count,
	unsigned long	jif_end)
{
	ssize_t	qty	= count;
	VADDR_T	dst	= dev->tx.io_reg_vaddr;
	u8*		src	= (u8*) buff;

	for (; count > 0; )
	{
		count	-= 1;
		writel(src[0], dst);
		src++;

		if (dev->tx.timeout_s)
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
*	Function:	gsc_write_pio_work_16_bit
*
*	Purpose:
*
*		Perform PIO based writes of 16-bit values.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		buff	The source for the data to write.
*
*		count	The number of bytes to write.
*
*		jif_end	The "jiffies" time at which we timeout.
*
*	Returned:
*
*		>= 0	The number of bytes written.
*		< 0		There was an error.
*
******************************************************************************/

#if defined(DEV_SUPPORTS_WRITE)
#if defined(GSC_WRITE_PIO_WORK) || defined(GSC_WRITE_PIO_WORK_16_BIT)
ssize_t gsc_write_pio_work_16_bit(
	dev_data_t*		dev,
	const char*		buff,
	ssize_t			count,
	unsigned long	jif_end)
{
	ssize_t	qty	= count;
	VADDR_T	dst	= dev->tx.io_reg_vaddr;
	u16*	src	= (u16*) buff;

	for (; count > 0; )
	{
		count	-= 2;
		writel(src[0], dst);
		src++;

		if (dev->tx.timeout_s)
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
*	Function:	gsc_write_pio_work_32_bit
*
*	Purpose:
*
*		Perform PIO based writes of 32-bit values.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		buff	The source for the data to write.
*
*		count	The number of bytes to write.
*
*		jif_end	The "jiffies" time at which we timeout.
*
*	Returned:
*
*		>= 0	The number of bytes written.
*		< 0		There was an error.
*
******************************************************************************/

#if defined(DEV_SUPPORTS_WRITE)
#if defined(GSC_WRITE_PIO_WORK) || defined(GSC_WRITE_PIO_WORK_32_BIT)
ssize_t gsc_write_pio_work_32_bit(
	dev_data_t*		dev,
	const char*		buff,
	ssize_t			count,
	unsigned long	jif_end)
{
	ssize_t	qty	= count;
	VADDR_T	dst	= dev->tx.io_reg_vaddr;
	u32*	src	= (u32*) buff;

	for (; count > 0; )
	{
		count	-= 4;
		writel(src[0], dst);
		src++;

		if (dev->tx.timeout_s)
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
*	Function:	gsc_write_pio_work
*
*	Purpose:
*
*		Perform PIO based writes of 32, 16 or 8 bit values.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		buff	The source for the data to write.
*
*		count	The number of bytes to write.
*
*		jif_end	The "jiffies" time at which we timeout.
*
*	Returned:
*
*		>= 0	The number of bytes written.
*		< 0		There was an error.
*
******************************************************************************/

#if defined(DEV_SUPPORTS_WRITE)
#if defined(GSC_WRITE_PIO_WORK)
ssize_t gsc_write_pio_work(
	dev_data_t*		dev,
	const char*		buff,
	ssize_t			count,
	unsigned long	jif_end)
{
	ssize_t	qty;

	if (dev->tx.data_size_bits == 32)
		qty	= gsc_write_pio_work_32_bit(dev, buff, count, jif_end);
	else if (dev->tx.data_size_bits == 16)
		qty	= gsc_write_pio_work_16_bit(dev, buff, count, jif_end);
	else if (dev->tx.data_size_bits == 8)
		qty	= gsc_write_pio_work_8_bit(dev, buff, count, jif_end);
	else
		qty	= -EINVAL;

	return(qty);
}
#endif
#endif


