// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/LINUX/16AI32SSC/driver/io.c $
// $Rev: 5857 $
// $Date$

#include "main.h"



/******************************************************************************
*
*	Function:	io_close
*
*	Purpose:
*
*		Cleanup the I/O stuff for the device as it is being closed.
*
*	Arguments:
*
*		dev	The data for the device of interest.
*
*	Returned:
*
*		None.
*
******************************************************************************/

void io_close(dev_data_t* dev)
{
	dev->rx.dma_channel	= NULL;
}



/******************************************************************************
*
*	Function:	io_create
*
*	Purpose:
*
*		Perform I/O based initialization as the driver is being loaded.
*
*	Arguments:
*
*		dev		The data for the device of interest.
*
*	Returned:
*
*		>= 0	The number of errors seen.
*
******************************************************************************/

int io_create(dev_data_t* dev)
{
	int	errs;

	dev->rx.bytes_per_sample	= 4;
	dev->rx.io_reg_offset		= GSC_REG_OFFSET(AI32SSC_GSC_IBDR);
	dev->rx.io_reg_vaddr		= dev->vaddr.gsc_ibdr_32;

	errs	= gsc_io_create(dev, &dev->rx, 256L * 1024 * 4);
	return(errs);
}



/******************************************************************************
*
*	Function:	io_destroy
*
*	Purpose:
*
*		Perform I/O based cleanup as the driver is being unloaded.
*
*	Arguments:
*
*		dev		The data for the device of interest.
*
*	Returned:
*
*		None.
*
******************************************************************************/

void io_destroy(dev_data_t* dev)
{
	io_close(dev);	// Just in case.
	gsc_io_destroy(dev, &dev->rx);
}



/******************************************************************************
*
*	Function:	io_open
*
*	Purpose:
*
*		Perform I/O based initialization as the device is being opened.
*
*	Arguments:
*
*		dev		The data for the device of interest.
*
*	Returned:
*
*		0		All went well.
*		< 0		The code for the error seen.
*
******************************************************************************/

int io_open(dev_data_t* dev)
{
	io_close(dev);	// Just in case.
	return(0);
}


