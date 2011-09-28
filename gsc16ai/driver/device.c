// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/LINUX/16AI32SSC/driver/device.c $
// $Rev: 9558 $
// $Date$

#include "main.h"



// variables	***************************************************************

const gsc_dev_id_t	dev_id_list[]	=
{
	// model		Vendor	Device	SubVen	SubDev	type

	{ "16AI32SSC",	0x10B5, 0x9056, 0x10B5, 0x3101,	GSC_DEV_TYPE_16AI32SSC	},	// initial criteria
	{ NULL }
};



//*****************************************************************************
static int _stricmp(const char* str1, const char* str2)
{
	int	c1;
	int	c2;
	int	ret;

	if ((str1 == NULL) && (str2 == NULL))
	{
		ret	= 0;
	}
	else if (str1 == NULL)
	{
		ret	= 1;
	}
	else if (str2 == NULL)
	{
		ret	= -1;
	}
	else
	{
		for (ret = 0;; str1++, str2++)
		{
			c1	= str1[0];
			c1	= (c1 < 'A') ? c1 : ((c1 > 'Z') ? c1 : c1 - 'A' + 'a');
			c2	= str2[0];
			c2	= (c2 < 'A') ? c2 : ((c2 > 'Z') ? c2 : c2 - 'A' + 'a');
			ret	= c2 - c1;

			if ((ret == 0) || (c1 == 0))
				break;
		}
	}

	return(ret);
}



//*****************************************************************************
static int _channels_compute(dev_data_t* dev)
{
	switch (dev->cache.gsc_bcfgr_32 & 0x30000)
	{
		case 0x00000:	dev->cache.channel_qty	= 32;	break;
		case 0x10000:	dev->cache.channel_qty	= 16;	break;
		case 0x20000:
		case 0x30000:
		default:		dev->cache.channel_qty	= -1;	break;
	}

	dev->cache.channels_max	= 32;
	return(0);
}



//*****************************************************************************
static int _master_clock_compute(dev_data_t* dev)
{
	switch (dev->cache.gsc_bcfgr_32 & 0xC0000)
	{
		case 0x00000:	dev->cache.master_clock	= 50000000L;	break;
		case 0x40000:
		case 0x80000:
		case 0xC0000:
		default:		dev->cache.master_clock	= -1;			break;
	}

	return(0);
}



//*****************************************************************************
static int _rate_generators_compute(dev_data_t* dev)
{
	dev->cache.rate_gen_fgen_max	= 200000L;
	dev->cache.rate_gen_fgen_min	= 1;
	dev->cache.rate_gen_nrate_mask	= 0xFFFF;
	dev->cache.rate_gen_nrate_max	= 0xFFFF;
	dev->cache.rate_gen_nrate_min	= 250;
	dev->cache.rate_gen_qty			= 2;
	return(0);
}



//*****************************************************************************
static int _sample_rates_compute(dev_data_t* dev)
{
	dev->cache.fsamp_max		= 200000L;
	dev->cache.fsamp_min		= 1L;
	return(0);
}



//*****************************************************************************
static int _fifo_size_compute(dev_data_t* dev)
{
	dev->cache.fifo_size	= _256K;
	return(0);
}



//*****************************************************************************
static int _initialize_compute(dev_data_t* dev)
{
	dev->cache.initialize_ms	= 3;
	return(0);
}



//*****************************************************************************
static int _auto_calibrate_compute(dev_data_t* dev)
{
	dev->cache.auto_cal_ms	= 1000;
	return(0);
}



//*****************************************************************************
static int _time_tag_compute(dev_data_t* dev)
{
	if ((dev->cache.gsc_bcfgr_32 & 0x300000) == 0x100000)
		dev->cache.time_tag_support	= 1;
	else
		dev->cache.time_tag_support	= 0;

	return(0);
}



/******************************************************************************
*
*	Function:	dev_check_id
*
*	Purpose:
*
*		Perform any additional checks to verify the identify of the board.
*		The 16AI32SSC has the same subsystem id as a few other boards. The
*		difference for this board is that BCR does not also appear at offsets
*		0x40 or 0x80.
*
*	Arguments:
*
*		dev		The data for the device of interest.
*
*	Returned:
*
*		0		All went well. This IS one of our devices.
*		> 0		There was a problem. This is NOT one of our devices.
*
******************************************************************************/

int dev_check_id(dev_data_t* dev)
{
	VADDR_T	_0x40_va;
	u32		_0x40_val;
	VADDR_T	_0x80_va;
	u32		_0x80_val;
	u32		bctlr;
	int		errs;

	_0x40_va	= GSC_VADDR(dev, 0x40);
	_0x80_va	= GSC_VADDR(dev, 0x80);

	bctlr		= readl(dev->vaddr.gsc_bctlr_32);
	_0x40_val	= readl(_0x40_va);
	_0x80_val	= readl(_0x80_va);

	if ((bctlr != _0x40_val) && (bctlr != _0x80_val))
		errs	= 0;	// This IS one of our boards.
	else
		errs	= 1;	// This is NOT one of our boards.

	return(errs);
}



/******************************************************************************
*
*	Function:	dev_device_create
*
*	Purpose:
*
*		Do everything needed to setup and use the given device.
*
*	Arguments:
*
*		dev		The structure to initialize.
*
*	Returned:
*
*		>=0		The number of errors seen.
*
******************************************************************************/

int dev_device_create(dev_data_t* dev)
{
	u32	dma;
	int	errs	= 0;

	for (;;)	// A convenience loop.
	{
		// Verify some macro contents.

		if (strcmp(DEV_NAME, AI32SSC_BASE_NAME))
		{
			errs	= 1;
			printk(	"%s: dev_device_create, line %d:"
					" DEV_NAME and AI32SSC_BASE_NAME are different.\n",
					DEV_NAME,
					__LINE__);
			printk(	"%s: dev_device_create, line %d:"
					" DEV_NAME is '%s' from main.h\n",
					DEV_NAME,
					__LINE__,
					DEV_NAME);
			printk(	"%s: dev_device_create, line %d:"
					" AI32SSC_BASE_NAME is '%s' from 16ai32ssc.h\n",
					DEV_NAME,
					__LINE__,
					AI32SSC_BASE_NAME);
			break;
		}

		if (_stricmp(DEV_NAME, DEV_MODEL))
		{
			errs	= 1;
			printk(	"%s: dev_device_create, line %d:"
					" DEV_NAME and DEV_MODEL do not agree.\n",
					DEV_NAME,
					__LINE__);
			printk(	"%s: dev_device_create, line %d:"
					" DEV_NAME is '%s' from main.h\n",
					DEV_NAME,
					__LINE__,
					DEV_NAME);
			printk(	"%s: dev_device_create, line %d:"
					" DEV_MODEL is '%s' from main.h\n",
					DEV_NAME,
					__LINE__,
					DEV_MODEL);
			break;
		}

		// Enable the PCI device.
		errs	= PCI_ENABLE_DEVICE(dev->pci);

		if (errs)
		{
			printk(	"%s: dev_device_create, line %d:"
					" PCI_ENABLE_DEVICE error\n",
					DEV_NAME,
					__LINE__);
			break;
		}

		pci_set_master(dev->pci);

		// Control accessto the device structure.
		gsc_sem_create(&dev->sem);

		// Access the BAR regions.
		errs	+= gsc_bar_create(dev->pci, 0, &dev->plx,  1, 0);	// memory
		errs	+= gsc_bar_create(dev->pci, 1, &dev->bar1, 0, 1);	// I/O
		errs	+= gsc_bar_create(dev->pci, 2, &dev->gsc,  1, 0);	// memory
		errs	+= gsc_bar_create(dev->pci, 3, &dev->bar3, 0, 0);	// any
		errs	+= gsc_bar_create(dev->pci, 4, &dev->bar4, 0, 0);	// any
		errs	+= gsc_bar_create(dev->pci, 5, &dev->bar5, 0, 0);	// any

		if (errs)
			break;

		dev->vaddr.plx_intcsr_32	= PLX_VADDR(dev, 0x68);
		dev->vaddr.plx_dmaarb_32	= PLX_VADDR(dev, 0xAC);
		dev->vaddr.plx_dmathr_32	= PLX_VADDR(dev, 0xB0);

		dev->vaddr.gsc_bctlr_32		= GSC_VADDR(dev, 0x00);	// Board Control Register
		dev->vaddr.gsc_icr_32		= GSC_VADDR(dev, 0x04);	// Interrupt Control Register
		dev->vaddr.gsc_ibdr_32		= GSC_VADDR(dev, 0x08);	// Input Buffer Data Register
		dev->vaddr.gsc_ibcr_32		= GSC_VADDR(dev, 0x0C);	// Input Buffer Control Register

		dev->vaddr.gsc_bufsr_32		= GSC_VADDR(dev, 0x18);	// Buffer Size Register

		dev->vaddr.gsc_acar_32		= GSC_VADDR(dev, 0x24);	// Active Channel Assignment Register
		dev->vaddr.gsc_bcfgr_32		= GSC_VADDR(dev, 0x28);	// Board Configuration Register

		dev->vaddr.gsc_smuwr_32		= GSC_VADDR(dev, 0x38);	// Scan Marker Upper Word Register
		dev->vaddr.gsc_smlwr_32		= GSC_VADDR(dev, 0x3C);	// Scan Marker Lower Word Register

		// These are the Time Tag registers
		dev->vaddr.gsc_ttcr_32		= GSC_VADDR(dev, 0x50);	// Time Tag Cinfiguration Register
		dev->vaddr.gsc_ttacmr_32	= GSC_VADDR(dev, 0x54);	// Time Tag Active Channel Mask Register

		// Start the cache.
		dev->cache.gsc_bcfgr_32		= readl(dev->vaddr.gsc_bcfgr_32);
		dev->cache.fw_ver			= dev->cache.gsc_bcfgr_32 & 0xFFF;

		errs	+= gsc_irq_create(dev);

		if (errs)
			break;

		errs	+= _channels_compute(dev);
		errs	+= _master_clock_compute(dev);
		errs	+= _rate_generators_compute(dev);
		errs	+= _sample_rates_compute(dev);
		errs	+= _fifo_size_compute(dev);
		errs	+= _initialize_compute(dev);
		errs	+= _auto_calibrate_compute(dev);
		errs	+= _time_tag_compute(dev);

		errs	+= io_create(dev);

		dma		= GSC_DMA_SEL_STATIC
				| GSC_DMA_CAP_DMA_READ
				| GSC_DMA_CAP_DMDMA_READ;
		errs	+= gsc_dma_create(dev, dma, dma);

		break;
	}

	return(errs);
}



/******************************************************************************
*
*	Function:	dev_device_destroy
*
*	Purpose:
*
*		Do everything needed to release the referenced device.
*
*	Arguments:
*
*		dev		The partial data for the device of interest.
*
*	Returned:
*
*		None.
*
******************************************************************************/

void dev_device_destroy(dev_data_t* dev)
{
	if (dev)
	{
		gsc_dma_destroy(dev);
		io_destroy(dev);
		gsc_irq_destroy(dev);

		// Tear down the BAR regions.
		gsc_bar_destroy(&dev->plx);
		gsc_bar_destroy(&dev->bar1);
		gsc_bar_destroy(&dev->gsc);
		gsc_bar_destroy(&dev->bar3);
		gsc_bar_destroy(&dev->bar4);
		gsc_bar_destroy(&dev->bar5);

		gsc_sem_destroy(&dev->sem);

		if (dev->pci)
			PCI_DISABLE_DEVICE(dev->pci);
	}
}


