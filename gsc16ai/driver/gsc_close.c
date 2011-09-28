// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_close.c $
// $Rev: 3890 $
// $Date$

#include "gsc_main.h"
#include "main.h"



//*****************************************************************************
int gsc_close(struct inode *inode, struct file *fp)
{
	dev_data_t*	dev;
	int			ret;

	for (;;)	// A convenience loop.
	{
		dev	= (dev_data_t *) fp->private_data;

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

		// Perform device specific CLOSE actions.
		ret	= dev_close(dev);

		// Complete
		dev->in_use			= 0;
		fp->private_data	= NULL;
		gsc_module_count_dec();
		gsc_sem_unlock(&dev->sem);
		break;
	}

	return(ret);
}



