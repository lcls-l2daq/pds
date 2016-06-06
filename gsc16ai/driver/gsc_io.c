// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_io.c $
// $Rev$
// $Date$

#include "main.h"



/******************************************************************************
*
*	Function:	_order_to_size
*
*	Purpose:
*
*		Convert an order value to its corresponding size in page sized
*		units.
*
*	Arguments:
*
*		order	The page size order to convert.
*
*	Returned:
*
*		>= 0	The order corresponding to the given size.
*
******************************************************************************/

static u32 _order_to_size(int order)
{
	u32	size	= PAGE_SIZE;

	// Limit sizes to 1G.

	for (; order > 0; order--)
	{
		if (size >= 0x40000000)
			break;

		size	*= 2;
	}

	return(size);
}



/******************************************************************************
*
*	Function:	_pages_free
*
*	Purpose:
*
*		Free memory allocated via _pages_alloc().
*
*	Arguments:
*
*		io	Parameters for the allocation to be freed.
*
*	Returned:
*
*		None.
*
******************************************************************************/

static void _pages_free(dev_io_t* io)
{
	s32		bytes;
	unsigned long	ul;

	if (io->adrs)
	{
		// Unreserve the pages before we return them to the MM.
		bytes	= io->bytes;
		ul	= (unsigned long) io->buf;

		for (; bytes >= PAGE_SIZE; bytes -= PAGE_SIZE, ul += PAGE_SIZE)
			PAGE_UNRESERVE(ul);

		// Free the memory.
		free_pages(io->adrs, io->order);
		io->adrs		= 0;
		io->buf			= NULL;
		io->bytes		= 0;
		io->order		= 0;
	}
}



/******************************************************************************
*
*	Function:	_size_to_order
*
*	Purpose:
*
*		Convert a size value to a corresponding page size order value.
*
*	Arguments:
*
*		size	The desired size in bytes.
*
*	Returned:
*
*		>= 0	The order corresponding to the given size.
*
******************************************************************************/

static int _size_to_order(u32 size)
{
	u32	bytes	= PAGE_SIZE;
	int	order	= 0;

	// Limit sizes to 1G.

	for (; bytes < 0x40000000;)
	{
		if (bytes >= size)
			break;

		order++;
		bytes	*= 2;
	}

	return(order);
}



/******************************************************************************
*
*	Function:	_pages_alloc
*
*	Purpose:
*
*		Allocate a block of memory that based on pages. The allocated
*		block is on a page boundary and the allocated size is rounded
*		upto a page boundary.
*
*	Arguments:
*
*		bytes	The desired number of bytes.
*
*		io		The structure through which the I/O transfer occurs and the
*				allocation is returned.
*
*	Returned:
*
*		None.
*
******************************************************************************/

static void _pages_alloc(u32 bytes, dev_io_t* io)
{
	unsigned long	adrs;
	u8				order;
	unsigned long	ul;

	if (bytes > MEM_ALLOC_LIMIT)
		bytes	= MEM_ALLOC_LIMIT;

	order	= _size_to_order(bytes);
	bytes	= _order_to_size(order);

	for (;;)
	{
		adrs	= __get_dma_pages(GFP_KERNEL | GFP_DMA | __GFP_NOWARN, order);

		if (adrs)
			break;

		if (order == 0)
			break;

		order--;
		bytes	/= 2;
	}

	if (adrs == 0)
	{
		io->adrs	= 0;
		io->buf		= NULL;
		io->bytes	= bytes;
		io->order	= 0;
		printk(	"%s: _pages_alloc, line %d:"
				" allocation failed\n",
				DEV_NAME,
				__LINE__);
	}
	else
	{
		io->adrs	= adrs;
		io->buf		= (u32*) adrs;
		io->bytes	= bytes;
		io->order	= order;

		// Mark the pages as reserved so the MM will leave'm alone.
		ul	= (unsigned long) io->buf;

		for (; bytes >= PAGE_SIZE; bytes -= PAGE_SIZE, ul += PAGE_SIZE)
			PAGE_RESERVE(ul);
	}
}



/******************************************************************************
*
*	Function:	gsc_io_create
*
*	Purpose:
*
*		Perform I/O based initialization as the driver is being loaded.
*
*	Arguments:
*
*		dev		The data for the device of interest.
*
*		io		The I/O structure of interest.
*
*		size	The desired size of the buffer in bytes.
*
*	Returned:
*
*		>= 0	The number of errors seen.
*
******************************************************************************/

int gsc_io_create(dev_data_t* dev, dev_io_t* io, size_t bytes)
{
	int	errs;

	gsc_sem_create(&io->sem);

	_pages_alloc(bytes, io);
	errs	= io->adrs ? 0 : 1;

	return(errs);
}



/******************************************************************************
*
*	Function:	gsc_io_destroy
*
*	Purpose:
*
*		Perform I/O based cleanup as the driver is being unloaded.
*
*	Arguments:
*
*		dev		The data for the device of interest.
*
*		io		The I/O structure of interest.
*
*	Returned:
*
*		None.
*
******************************************************************************/

void gsc_io_destroy(dev_data_t* dev, dev_io_t* io)
{
	if (io->buf)
		_pages_free(io);

	io->buf	= NULL;
}


