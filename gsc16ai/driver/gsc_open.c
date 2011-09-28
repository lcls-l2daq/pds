// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_open.c $
// $Rev: 3890 $
// $Date$

#include "gsc_main.h"
#include "main.h"



//*****************************************************************************
int gsc_open(struct inode *inode, struct file *fp)
{
	#define	_RW_	(FMODE_READ | FMODE_WRITE)

	dev_data_t*	dev;
	int			ret;

	for (;;)	// We'll use a loop for convenience.
	{
		if ((fp->f_mode & _RW_) != _RW_)
		{
			// Read and write access are both required.
			ret	= -EINVAL;
			break;
		}

		dev	= gsc_dev_data_t_locate(inode);

		if (dev == NULL)
		{
			// The referenced device doesn't exist.
			ret	= -ENODEV;
			break;
		}

		ret	= gsc_sem_lock(&dev->sem);

		if (ret)
		{
			// We didn't get the semaphore.
			break;
		}

		if (dev->in_use)
		{
			// The device is already being used.
			ret	= -EBUSY;
			gsc_sem_unlock(&dev->sem);
			break;
		}

		ret	= gsc_module_count_inc();

		if (ret)
		{
			// We coundn't increase the usage count.
			up(&dev->sem);
			break;
		}

		// Perform device specific OPEN actions.
		ret	= dev_open(dev);

		if (ret)
		{
			// There was a problem here.
			dev_close(dev);
			gsc_sem_unlock(&dev->sem);
			gsc_module_count_dec();
			break;
		}

		dev->in_use			= 1;	// Mark as being open/in use.
		fp->private_data	= dev;

		gsc_sem_unlock(&dev->sem);
		break;
	}

	return(ret);
}


