// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_ioctl.c $
// $Rev: 9009 $
// $Date$

#include "gsc_main.h"
#include "main.h"



// variables	***************************************************************

static	int	_list_qty;



/******************************************************************************
*
*	Function:	_common_ioctl
*
*	Purpose:
*
*		Implement common functionality for the various ioctl() methods.
*
*	Arguments:
*
*		inode	The device inode structure.
*
*		fp		A pointer to the file structure.
*
*		cmd		The service being requested.
*
*		arg		The service specific argument.
*
*	Returned:
*
*		0		All went well.
*		< 0		There was an error.
*
******************************************************************************/

static int _common_ioctl(
		struct inode*	inode,
		struct file*	fp,
		unsigned int	cmd,
		unsigned long	arg)
{
	union
	{
		char		buf[128];
		gsc_reg_t	reg;
		__u32		var_u32;
		gsc_vpd_t	vpd;
	} buf;

	dev_data_t*			dev		= (dev_data_t*) fp->private_data;
	unsigned long		dir;
	gsc_ioctl_service_t	func;
	unsigned long		index;
	int					ret;
	unsigned long		size;
	unsigned long		type;
	unsigned long		ul;

	for (;;)	// A convenience loop.
	{
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

		type	= _IOC_TYPE(cmd);

		if (type != GSC_IOCTL)
		{
			// The IOCTL code isn't ours.
			ret	= -EINVAL;
			gsc_sem_unlock(&dev->sem);
			break;
		}

		index	= _IOC_NR(cmd);

		if (index >= _list_qty)
		{
			// The IOCTL service is unrecognized.
			ret	= -EINVAL;
			gsc_sem_unlock(&dev->sem);
			break;
		}

		if (dev_ioctl_list[index].func == NULL)
		{
			// The IOCTL service is unimplemented.
			ret	= -EINVAL;
			gsc_sem_unlock(&dev->sem);
			break;
		}

		size	= _IOC_SIZE(cmd);

		if (size != _IOC_SIZE(dev_ioctl_list[index].cmd))
		{
			// The IOCTL data size is invalid.
			ret	= -EINVAL;
			gsc_sem_unlock(&dev->sem);
			break;
		}

		if (size > sizeof(buf))
		{
			// The buffer is too small.
			printk(	"%s: _common_ioctl, line %d:"
					" 'buf' is too small:"
					" is %ld, need %ld\n",
					DEV_NAME,
					__LINE__,
					(long) sizeof(buf),
					(long) size);
			ret	= -EFAULT;
			gsc_sem_unlock(&dev->sem);
			break;
		}

		dir	= _IOC_DIR(cmd);

		if (dir & _IOC_WRITE)
		{
			ul	= access_ok(VERIFY_READ, arg, size);

			if (ul == 0)
			{
				// We can't read from the user's memory.
				ret	= -EFAULT;
				gsc_sem_unlock(&dev->sem);
				break;
			}

			ul	= copy_from_user(&buf, (void*) arg, size);

			if (ul)
			{
				// We couldn't read from the user's memory.
				ret	= -EFAULT;
				gsc_sem_unlock(&dev->sem);
				break;
			}
		}

		if (dir & _IOC_READ)
		{
			ul	= access_ok(VERIFY_WRITE, arg, size);

			if (ul == 0)
			{
				// We can't write to the user's memory.
				ret	= -EFAULT;
				gsc_sem_unlock(&dev->sem);
				break;
			}
		}

		// Process the IOCTL command.
		func	= dev_ioctl_list[index].func;

		if (size)
			ret		= func(dev, &buf);
		else
			ret		= func(dev, (void*) arg);

		if (ret)
		{
			// The service failed.
			gsc_sem_unlock(&dev->sem);
			break;
		}

		if (dir & _IOC_READ)
		{
			ul	= copy_to_user((void*) arg, &buf, size);

			if (ul)
			{
				// We couldn't write to the user's memory.
				ret	= -EFAULT;
				gsc_sem_unlock(&dev->sem);
				break;
			}
		}

		ret	= 0;
		gsc_sem_unlock(&dev->sem);
		break;
	}

	return(ret);
}



/******************************************************************************
*
*	Function:	gsc_ioctl
*
*	Purpose:
*
*		Implement the ioctl() procedure.
*
*	Arguments:
*
*		inode	This is the device inode structure.
*
*		fp		This is the file structure.
*
*		cmd		The desired IOCTL service.
*
*		arg		The optional service specific argument.
*
*	Returned:
*
*		0		All went well.
*		< 0		There was a problem.
*
******************************************************************************/

int gsc_ioctl(
	struct inode*	inode,
	struct file*	fp,
	unsigned int	cmd,
	unsigned long	arg)
{
	int	ret;

	ret	= _common_ioctl(inode, fp, cmd, arg);
	return(ret);
}



/******************************************************************************
*
*	Function:	gsc_ioctl_compat
*
*	Purpose:
*
*		Implement the compat_ioctl() method.
*
*	Arguments:
*
*		fp	A pointer to the file structure.
*
*		cmd	The service being requested.
*
*		arg	The service specific argument.
*
*	Returned:
*
*		0	All went well.
*		< 0	There was an error.
*
******************************************************************************/

#if HAVE_COMPAT_IOCTL
long gsc_ioctl_compat(struct file* fp, unsigned int cmd, unsigned long arg)
{
	int	ret;

	// No additional locking needed as we use a per device lock.
	ret	= _common_ioctl(fp->f_dentry->d_inode, fp, cmd, arg);
	return(ret);
}
#endif



/******************************************************************************
*
*	Function:	gsc_ioctl_unlocked
*
*	Purpose:
*
*		Implement the unlocked_ioctl() method.
*
*	Arguments:
*
*		fp	A pointer to the file structure.
*
*		cmd	The service being requested.
*
*		arg	The service specific argument.
*
*	Returned:
*
*		0	All went well.
*		< 0	There was an error.
*
******************************************************************************/

#if HAVE_UNLOCKED_IOCTL
long gsc_ioctl_unlocked(struct file* fp, unsigned int cmd, unsigned long arg)
{
	int	ret;

	// As of 8/13/2008 all data types are sized identically for 32-bit
	// and 64-bit environments.
	// No additional locking needed as we use a per device lock.
	ret	= _common_ioctl(fp->f_dentry->d_inode, fp, cmd, arg);
	return(ret);
}
#endif



/******************************************************************************
*
*	Function:	gsc_ioctl_init
*
*	Purpose:
*
*		Perform any work needed when starting the driver.
*
*	Arguments:
*
*		None.
*
*	Returned:
*
*		0	All went well.
*		!0	There was a problem.
*
******************************************************************************/

int gsc_ioctl_init(void)
{
	#if (IOCTL_32BIT_SUPPORT == GSC_IOCTL_32BIT_TRANSLATE)
	int	test;
	#endif

	int	errs	= 0;
	int	i;

	for (;;)	// A convenience loop.
	{
		// Compute the size of the IOCTL service list.

		for (i = 0;; i++)
		{
			if ((dev_ioctl_list[i].cmd == -1)	&&
				(dev_ioctl_list[i].func == NULL))
			{
				_list_qty	= i;
				break;
			}
		}

		// Verify that the IOCTL service list is in order.

		for (i = 0; i < _list_qty; i++)
		{
			if (_IOC_NR(dev_ioctl_list[i].cmd) != i)
			{
				printk(	"%s: gsc_ioctl_init: line %d:"
						" IOCTL list #%d is out of order.\n",
						DEV_NAME,
						__LINE__,
						i);
				errs	= 1;
				break;
			}
		}

		if (errs)
			break;

		// Perform the steps needed for support of 32-bit applications
		// under a 64-bit OS.

		gsc_global.ioctl_32bit	= IOCTL_32BIT_SUPPORT;

#if (IOCTL_32BIT_SUPPORT == GSC_IOCTL_32BIT_TRANSLATE)

		// Register each of our defined services.

		for (i = 0; i < _list_qty; i++)
		{
			test	= register_ioctl32_conversion(
						dev_ioctl_list[i].cmd,
						NULL);	// NULL = use 64-bit handler

			if (test)
			{
				// This service has already been registered by
				// someone else.
				printk(	"%s: gsc_ioctl_init, line %d:"
						" Duplicate 'cmd' id at index %d."
						" 32-bit IOCTL support disabled.\n",
						DEV_NAME,
						__LINE__,
						i);
				gsc_global.ioctl_32bit	= GSC_IOCTL_32BIT_DISABLED;

				// Unregister those that were registered.

				for (; i >= 0; i--)
					unregister_ioctl32_conversion(dev_ioctl_list[i].cmd);

				break;
			}
		}

#endif

		break;
	}

	return(errs);
}



/******************************************************************************
*
*	Function:	gsc_ioctl_reset
*
*	Purpose:
*
*		Perform any clean-up upon unloading the module.
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

void gsc_ioctl_reset(void)
{
#if (IOCTL_32BIT_SUPPORT == GSC_IOCTL_32BIT_TRANSLATE)
	int	i;

	if (gsc_global.ioctl_32bit)
	{
		for (i = 0; i < _list_qty; i++)
			unregister_ioctl32_conversion(dev_ioctl_list[i].cmd);
	}
#endif
}


