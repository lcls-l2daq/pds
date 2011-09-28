// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_init.c $
// $Rev: 9225 $
// $Date$

#include "gsc_main.h"
#include "main.h"



// variables	***************************************************************

gsc_global_t	gsc_global;



/******************************************************************************
*
*	Function:	_check_id
*
*	Purpose:
*
*		Check to see if the referenced device is one we support. If it is,
*		then fill in identity fields for the device structure given.
*
*	Arguments:
*
*		pci		The structure referencing the PCI device of interest.
*
*		dev		The device structure were we put identify data.
*
*	Returned:
*
*		0		We don't support this device.
*		1		We do support this device.
*
******************************************************************************/

static int _check_id(struct pci_dev* pci, dev_data_t* dev)
{
	int	found	= 0;
	int	i;
	u16	sdidr;
	u16	svidr;

	pci_read_config_word(pci, 0x2C, &svidr);
	pci_read_config_word(pci, 0x2E, &sdidr);

	for (i = 0; dev_id_list[i].model; i++)
	{
		if ((dev_id_list[i].vendor		== pci->vendor)	&&
			(dev_id_list[i].device		== pci->device)	&&
			(dev_id_list[i].sub_vendor	== svidr)		&&
			(dev_id_list[i].sub_device	== sdidr))
		{
			found			= 1;
			dev->model		= dev_id_list[i].model;
			dev->board_type	= dev_id_list[i].type;
			break;

			// It is possible for the model and type to change once the device
			// specific code has a chance to examine the board more closely.
		}
	}

#if DEV_PCI_ID_SHOW
	printk(	"ID: %04lX %04lX %04lX %04lX",
			(long) pci->vendor,
			(long) pci->device,
			(long) svidr,
			(long) sdidr);

	if (found)
	{
		printk(" <--- %s, type %d", dev->model, dev->board_type);
	}

	printk("\n");
#endif

	return(found);
}



/******************************************************************************
*
*	Function:	_dev_data_t_add
*
*	Purpose:
*
*		Add a device to our device list.
*
*	Arguments:
*
*		dev		A pointer to the structure to add.
*
*	Returned:
*
*		>=0		The number of errors seen.
*
******************************************************************************/

static int _dev_data_t_add(dev_data_t* dev)
{
	int	errs;
	int	i;

	for (;;)	// A convenience loop.
	{
		for (i = 0; i < ARRAY_ELEMENTS(gsc_global.dev_list); i++)
		{
			if (gsc_global.dev_list[i] == NULL)
				break;
		}

		if (i >= ARRAY_ELEMENTS(gsc_global.dev_list))
		{
			// We don't have enough room to add the device.
			errs	= 1;
			printk(	"%s: _dev_data_t_add, line %d:"
					" Too many devices were found. The limit is %d.\n",
					DEV_NAME,
					__LINE__,
					(int) ARRAY_ELEMENTS(gsc_global.dev_list));
			break;
		}

		gsc_global.dev_list[i]	= dev;
		dev->board_index		= i;
		errs					= 0;
		break;
	}

	return(errs);
}



/******************************************************************************
*
*	Function:	_dev_data_t_delete
*
*	Purpose:
*
*		Delete the given structure from the global list.
*
*	Arguments:
*
*		dev		The structure to delete.
*
*	Returned:
*
*		None.
*
******************************************************************************/

static void _dev_data_t_delete(dev_data_t* dev)
{
	int	i;

	for (i = 0; i < ARRAY_ELEMENTS(gsc_global.dev_list); i++)
	{
		if (gsc_global.dev_list[i] == dev)
		{
			gsc_global.dev_list[i]	= NULL;
			break;
		}
	}
}



/******************************************************************************
*
*	Function:	gsc_dev_data_t_locate
*
*	Purpose:
*
*		Get a pointer to the device structure referenced by the given i_node
*		structure.
*
*	Arguments:
*
*		inode	This is the device inode structure.
*
*	Returned:
*
*		NULL	The respective device can't be accessed.
*		else	A pointer to the respective device.
*
******************************************************************************/

dev_data_t* gsc_dev_data_t_locate(struct inode* inode)
{
	dev_data_t*		dev;
	unsigned int	index	= MINOR(inode->i_rdev);

	if (index >= ARRAY_ELEMENTS(gsc_global.dev_list))
		dev	= NULL;
	else
		dev	= gsc_global.dev_list[index];

	return(dev);
}



/******************************************************************************
*
*	Function:	cleanup_module
*
*	Purpose:
*
*		Clean things up when the kernel is about to unload the module.
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

void cleanup_module(void)
{
	int	i;

	if (gsc_global.major_number >= 0)
	{
		unregister_chrdev(gsc_global.major_number, DEV_NAME);
		gsc_global.major_number	= -1;
	}

	for (i = 0; i < ARRAY_ELEMENTS(gsc_global.dev_list); i++)
	{
		if (gsc_global.dev_list[i])
		{
			dev_device_destroy(gsc_global.dev_list[i]);
			kfree(gsc_global.dev_list[i]);
			gsc_global.dev_list[i]	= NULL;
		}
	}

	gsc_ioctl_reset();
	gsc_proc_stop();
	gsc_global.dev_qty	= 0;

	if (gsc_global.driver_loaded)
	{
		gsc_global.driver_loaded	= 0;
		printk(	"%s: driver version %s successfully removed.\n",
				DEV_NAME,
				GSC_DRIVER_VERSION);
	}
}



/******************************************************************************
*
*	Function:	init_module
*
*	Purpose:
*
*		Initialize the driver upon loading.
*
*	Arguments:
*
*		None.
*
*	Returned:
*
*		0	All went well.
*		< 0	There was an error.
*
******************************************************************************/

int init_module(void)
{
	dev_data_t*		dev;
	int				errs	= 0;
	int				found;
	struct pci_dev*	pci;
	int				ret		= 0;
	int				size	= sizeof(dev_data_t);
	dev_data_t*		tmp		= NULL;

	sprintf(gsc_global.built, "%s, %s", __DATE__, __TIME__);
	gsc_global.major_number	= -1;
	printk(	"%s: driver loading: version %s, built %s\n",
			DEV_NAME,
			GSC_DRIVER_VERSION,
			gsc_global.built);

	// Locate the devices and add them to our list.

	PCI_DEVICE_LOOP(pci)
	{
		if (tmp == NULL)
		{
			tmp	= kmalloc(size, GFP_KERNEL);

			if (tmp == NULL)
			{
				printk(	"%s: init_module, line %d:"
						" kmalloc(%ld) failed.\n",
						DEV_NAME,
						__LINE__,
						(long) size);
				break;
			}
		}

		memset(tmp, 0, size);
		found	= _check_id(pci, tmp);

		if (found == 0)
			continue;

		dev	= kmalloc(size, GFP_KERNEL);

		if (dev == NULL)
		{
			printk(	"%s: init_module, line %d:"
					" kmalloc(%ld) failed.\n",
					DEV_NAME,
					__LINE__,
					(long) size);
			continue;
		}

		memcpy(dev, tmp, size);
		errs	= _dev_data_t_add(dev);

		if (errs)
		{
			kfree(dev);
			dev	= NULL;
			continue;
		}

		dev->pci	= pci;
		errs		= dev_device_create(dev);

		if (errs == 0)
			errs	= dev_check_id(dev);

		if (errs)
		{
			dev_device_destroy(dev);
			_dev_data_t_delete(dev);
			kfree(dev);
			dev	= NULL;
			continue;
		}

		printk(	"%s: device loaded: %s\n",
				DEV_NAME,
				gsc_global.dev_list[gsc_global.dev_qty]->model);
		gsc_global.dev_qty++;
		dev	= NULL;
	}

	// Perform initialization following device discovery.

	for (;;)	// A convenience loop.
	{
		if (gsc_global.dev_qty <= 0)
		{
			ret	= -ENODEV;
			cleanup_module();
			printk(	"%s: driver load failure: version %s, built %s\n",
					DEV_NAME,
					GSC_DRIVER_VERSION,
					gsc_global.built);
			break;
		}

		gsc_global.fops.open	= gsc_open;
		gsc_global.fops.release	= gsc_close;
		gsc_global.fops.ioctl	= gsc_ioctl;
		gsc_global.fops.read	= gsc_read;
		gsc_global.fops.write	= gsc_write;
		IOCTL_SET_COMPAT(&gsc_global.fops, gsc_ioctl_compat);
		IOCTL_SET_UNLOCKED(&gsc_global.fops, gsc_ioctl_unlocked);
		SET_MODULE_OWNER(&gsc_global.fops);

		gsc_global.major_number	= register_chrdev(	0,
													DEV_NAME,
													&gsc_global.fops);

		if (gsc_global.major_number < 0)
		{
			ret	= -ENODEV;
			printk(	"%s: init_module, line %d:"
					" register_chrdev failed.\n",
					DEV_NAME,
					__LINE__);
			cleanup_module();
			break;
		}

		errs	= gsc_proc_start();

		if (errs)
		{
			ret	= -ENODEV;
			cleanup_module();
			break;
		}

		errs	= gsc_ioctl_init();

		if (errs)
		{
			ret	= -ENODEV;
			cleanup_module();
			break;
		}

		gsc_global.driver_loaded	= 1;
		printk(	"%s: driver loaded: version %s, built %s\n",
				DEV_NAME,
				GSC_DRIVER_VERSION,
				gsc_global.built);
		ret	= 0;
		break;
	}

	if (tmp)
	{
		kfree(tmp);
		tmp	= NULL;
	}

	return(ret);
}


