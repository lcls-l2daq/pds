// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_bar.c $
// $Rev: 3175 $
// $Date$

#include "gsc_main.h"
#include "main.h"



/******************************************************************************
*
*	Function:	gsc_bar_create
*
*	Purpose:
*
*		Initialize the given structure according the the BAR index given.
*
*	Arguments:
*
*		pci		The PCI structure for this device.
*
*		index	The BAR index to access.
*
*		bar		The structure to initialize.
*
*		mem		Must this BAR be memory mapped?
*
*		io		Must this BAR be I/O mapped?
*
*	Returned:
*
*		>=0		The number of errors seen.
*
******************************************************************************/

int gsc_bar_create(struct pci_dev* pci, int index, gsc_bar_t* bar, int mem, int io)
{
	int		errs	= 0;
	int		i;
	void*	vp		= NULL;

	memset(bar, 0,sizeof(gsc_bar_t));
	bar->index		= index;
	bar->offset		= 0x10 + 4 * index;
	bar->flags		= PCI_BAR_FLAGS(pci, index);
	bar->phys_adrs	= PCI_BAR_ADDRESS(pci, index);
	bar->size		= PCI_BAR_SIZE(pci, index);
	pci_read_config_dword(pci, bar->offset, &bar->reg);

	if ((bar->phys_adrs == 0) || (bar->size == 0))
	{
		// This region is unmapped.

		if (mem)
		{
			// This BAR region must be memory mapped.
			errs	= 1;
			printk(	"%s: gsc_bar_create, line %d:"
					" BAR%d is unmapped and MUST be memory mapped\n",
					DEV_NAME,
					__LINE__,
					index);
		}

		if (io)
		{
			// This BAR region must be I/O mapped.
			errs	= 1;
			printk(	"%s: gsc_bar_create, line %d:"
					" BAR%d is unmapped and MUST be I/O mapped\n",
					DEV_NAME,
					__LINE__,
					index);
		}
	}
	else if (bar->flags & REGION_TYPE_IO_BIT)
	{
		// This region is I/O mapped.
		bar->io_mapped	= 1;
		i				= REGION_IO_CHECK(bar->phys_adrs, bar->size);

		if (i == 0)
			vp	= REGION_IO_REQUEST(bar->phys_adrs, bar->size, DEV_NAME);

		if ((i == 0) && (vp))
		{
			// All went well.
			errs			= 0;
			bar->requested	= 1;
			bar->vaddr		= (void*) (VADDR_T) bar->phys_adrs;
		}

		if (mem)
		{
			// This BAR region must be memory mapped.
			errs	= 1;
			printk(	"%s: gsc_bar_create, line %d:"
					" BAR%d is I/O mapped and MUST be memory mapped\n",
					DEV_NAME,
					__LINE__,
					index);
		}

		if ((io) && (bar->vaddr == 0))
		{
			// At present this must be successful.
			errs	= 1;
			printk(	"%s: gsc_bar_create, line %d:"
					" BAR%d access failed.\n",
					DEV_NAME,
					__LINE__,
					index);
		}
	}
	else
	{
		// This region is memory mapped.
		bar->io_mapped	= 0;
		i				= REGION_MEM_CHECK(bar->phys_adrs, bar->size);

		if (i == 0)
			vp	= REGION_MEM_REQUEST(bar->phys_adrs, bar->size, DEV_NAME);

		if ((i == 0) && (vp))
		{
			bar->requested	= 1;
			bar->vaddr		= ioremap(bar->phys_adrs, bar->size);
		}

		if (io)
		{
			// This BAR region must be I/O mapped.
			errs	= 1;
			printk(	"%s: gsc_bar_create, line %d:"
					" BAR%d is memory mapped and MUST be I/O mapped\n",
					DEV_NAME,
					__LINE__,
					index);
		}

		if ((mem) && (bar->vaddr == 0))
		{
			// At present this must be successful.
			errs	= 1;
			printk(	"%s: gsc_bar_create, line %d:"
					" BAR%d access failed.\n",
					DEV_NAME,
					__LINE__,
					index);
		}
	}

#if DEV_BAR_SHOW
	printk(	"BAR%d"
			": Reg 0x%08lX"
			", Map %s"
			", Adrs 0x%08lX"
			", Size %4ld"
			", vaddr 0x%lX"
			"\n",
			(int) index,
			(long) bar->reg,
			bar->size ? (bar->io_mapped ? "I/O" : "mem") : "N/A",
			(long) bar->phys_adrs,
			(long) bar->size,
			(long) bar->vaddr);
#endif

	return(errs);
}



/******************************************************************************
*
*	Function:	gsc_bar_destroy
*
*	Purpose:
*
*		Release the given BAR region and its resources.
*
*	Arguments:
*
*		bar		The structure for the BAR to release.
*
*	Returned:
*
*		None.
*
******************************************************************************/

void gsc_bar_destroy(gsc_bar_t* bar)
{
	if (bar->requested == 0)
	{
	}
	else if (bar->io_mapped)
	{
		REGION_IO_RELEASE(bar->phys_adrs, bar->size);
	}
	else
	{
		if (bar->vaddr)
			iounmap(bar->vaddr);

		REGION_MEM_RELEASE(bar->phys_adrs, bar->size);
	}
}



